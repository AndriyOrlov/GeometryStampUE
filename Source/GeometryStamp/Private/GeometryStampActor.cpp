#include "GeometryStampActor.h"

#include "GeometryStampPreset.h"

#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/PlatformTime.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshNormals.h"
#include "DynamicMeshToMeshDescription.h"
#include "UDynamicMesh.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Factories/DataAssetFactory.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "ObjectTools.h"
#include "PhysicsEngine/BodySetup.h"
#include "ScopedTransaction.h"
#include "UObject/Package.h"

using UE::Geometry::FDynamicMesh3;
using UE::Geometry::FMeshNormals;

namespace
{
struct FGeometryStampHeightSampler
{
    bool Initialize(UTexture2D* Texture, EGeometryStampHeightChannel InChannel)
    {
        if (!Texture || !Texture->Source.IsValid())
        {
            return false;
        }

        Width = Texture->Source.GetSizeX();
        Height = Texture->Source.GetSizeY();
        Format = Texture->Source.GetFormat();
        Channel = InChannel;

        if (Width <= 0 || Height <= 0 || (Format != TSF_G8 && Format != TSF_BGRA8))
        {
            return false;
        }

        return Texture->Source.GetMipData(Data, 0);
    }

    bool Sample(const FVector2D& UV, const FVector2D& Tiling, float& OutHeight) const
    {
        if (Data.IsEmpty())
        {
            return false;
        }

        FVector2D TiledUV = (UV - FVector2D(0.5, 0.5)) * Tiling + FVector2D(0.5, 0.5);
        TiledUV.X = FMath::Frac(TiledUV.X + 1000.0);
        TiledUV.Y = FMath::Frac(TiledUV.Y + 1000.0);

        const int32 PixelX = FMath::Clamp(FMath::RoundToInt(TiledUV.X * (Width - 1)), 0, Width - 1);
        const int32 PixelY = FMath::Clamp(FMath::RoundToInt((1.0 - TiledUV.Y) * (Height - 1)), 0, Height - 1);

        if (Format == TSF_G8)
        {
            OutHeight = Data[PixelY * Width + PixelX] / 255.0f;
            return true;
        }

        const int64 PixelOffset = static_cast<int64>(PixelY * Width + PixelX) * 4;
        const float B = Data[PixelOffset] / 255.0f;
        const float G = Data[PixelOffset + 1] / 255.0f;
        const float R = Data[PixelOffset + 2] / 255.0f;
        const float A = Data[PixelOffset + 3] / 255.0f;

        switch (Channel)
        {
        case EGeometryStampHeightChannel::Red:
            OutHeight = R;
            break;
        case EGeometryStampHeightChannel::Green:
            OutHeight = G;
            break;
        case EGeometryStampHeightChannel::Blue:
            OutHeight = B;
            break;
        case EGeometryStampHeightChannel::Alpha:
            OutHeight = A;
            break;
        case EGeometryStampHeightChannel::Luminance:
            OutHeight = R * 0.2126f + G * 0.7152f + B * 0.0722f;
            break;
        default:
            return false;
        }

        return true;
    }

private:
    int32 Width = 0;
    int32 Height = 0;
    ETextureSourceFormat Format = TSF_Invalid;
    EGeometryStampHeightChannel Channel = EGeometryStampHeightChannel::Red;
    TArray64<uint8> Data;
};

bool IsSurfaceAngleAllowed(const FVector& TraceDirection, const FVector& ImpactNormal, double MaxAngleDegrees)
{
    const double MaxAngle = FMath::Clamp(MaxAngleDegrees, 0.0, 90.0);
    const double Alignment = FVector::DotProduct((-TraceDirection).GetSafeNormal(), ImpactNormal.GetSafeNormal());
    const double MinimumAlignment = MaxAngle >= 90.0
        ? 0.0
        : FMath::Cos(FMath::DegreesToRadians(MaxAngle));
    return Alignment >= MinimumAlignment;
}

bool IsTriangleSurfaceAngleAllowed(
    const FVector& TraceDirection,
    const FVector& A,
    const FVector& B,
    const FVector& C,
    double MaxAngleDegrees)
{
    const FVector TriangleNormal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
    return !TriangleNormal.IsNearlyZero()
        && (IsSurfaceAngleAllowed(TraceDirection, TriangleNormal, MaxAngleDegrees)
            || IsSurfaceAngleAllowed(TraceDirection, -TriangleNormal, MaxAngleDegrees));
}

bool BuildStaticMeshFromDynamicMesh(
    UStaticMesh& StaticMesh,
    const FDynamicMesh3& DynamicMesh,
    UMaterialInterface* Material,
    bool bEnableNanite)
{
    if (DynamicMesh.TriangleCount() == 0)
    {
        return false;
    }

    StaticMesh.SetNumSourceModels(1);
    FMeshBuildSettings& BuildSettings = StaticMesh.GetSourceModel(0).BuildSettings;
    BuildSettings.bRecomputeNormals = false;
    BuildSettings.bRecomputeTangents = true;
    BuildSettings.bGenerateLightmapUVs = false;
    BuildSettings.bUseFullPrecisionUVs = true;

    StaticMesh.CreateMeshDescription(0);
    FMeshDescription* MeshDescription = StaticMesh.GetMeshDescription(0);
    if (!MeshDescription)
    {
        return false;
    }

    FDynamicMeshToMeshDescription Converter;
    Converter.Convert(&DynamicMesh, *MeshDescription, false);
    StaticMesh.CommitMeshDescription(0);

    FStaticMaterial StaticMaterial;
    StaticMaterial.MaterialInterface = Material ? Material : UMaterial::GetDefaultMaterial(MD_Surface);
    StaticMaterial.MaterialSlotName = TEXT("GeometryStampMaterial");
    StaticMaterial.ImportedMaterialSlotName = StaticMaterial.MaterialSlotName;
    StaticMaterial.UVChannelData = FMeshUVChannelInfo(1.0f);
    StaticMesh.GetStaticMaterials().Reset();
    StaticMesh.GetStaticMaterials().Add(StaticMaterial);

    StaticMesh.NaniteSettings.bEnabled = bEnableNanite;
    StaticMesh.CreateBodySetup();
    StaticMesh.GetBodySetup()->CollisionTraceFlag = CTF_UseComplexAsSimple;
    StaticMesh.MarkPackageDirty();
    StaticMesh.PostEditChange();
    return true;
}
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGeometryStampSurfaceAngleTest,
    "GeometryStamp.Projection.SurfaceAngle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGeometryStampSurfaceAngleTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Horizontal surfaces are accepted"), IsSurfaceAngleAllowed(FVector::DownVector, FVector::UpVector, 85.0));
    TestFalse(TEXT("Vertical surfaces are rejected at 85 degrees"), IsSurfaceAngleAllowed(FVector::DownVector, FVector::ForwardVector, 85.0));
    TestTrue(TEXT("Ninety degrees allows vertical surfaces"), IsSurfaceAngleAllowed(FVector::DownVector, FVector::ForwardVector, 90.0));
    TestTrue(TEXT("Flat generated triangles are accepted"), IsTriangleSurfaceAngleAllowed(
        FVector::DownVector, FVector::ZeroVector, FVector::ForwardVector, FVector::RightVector, 85.0));
    TestFalse(TEXT("Vertical generated triangles are rejected"), IsTriangleSurfaceAngleAllowed(
        FVector::DownVector, FVector::ZeroVector, FVector::UpVector, FVector::RightVector, 85.0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGeometryStampStaticMeshBakeTest,
    "GeometryStamp.Bake.StaticMeshConversion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGeometryStampStaticMeshBakeTest::RunTest(const FString& Parameters)
{
    FDynamicMesh3 TestMesh(true, true, true, false);
    const int32 V0 = TestMesh.AppendVertex(FVector3d(0.0, 0.0, 0.0));
    const int32 V1 = TestMesh.AppendVertex(FVector3d(100.0, 0.0, 0.0));
    const int32 V2 = TestMesh.AppendVertex(FVector3d(0.0, 100.0, 0.0));
    TestMesh.SetVertexUV(V0, FVector2f(0.0f, 0.0f));
    TestMesh.SetVertexUV(V1, FVector2f(1.0f, 0.0f));
    TestMesh.SetVertexUV(V2, FVector2f(0.0f, 1.0f));
    TestMesh.AppendTriangle(V0, V2, V1);
    FMeshNormals::QuickComputeVertexNormals(TestMesh, false);

    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(GetTransientPackage());
    TestTrue(TEXT("Dynamic mesh converts to a static mesh"), BuildStaticMeshFromDynamicMesh(*StaticMesh, TestMesh, nullptr, true));
    TestTrue(TEXT("Nanite option is preserved"), StaticMesh->NaniteSettings.bEnabled);
    TestNotNull(TEXT("LOD0 mesh description exists"), StaticMesh->GetMeshDescription(0));
    if (const FMeshDescription* MeshDescription = StaticMesh->GetMeshDescription(0))
    {
        TestEqual(TEXT("LOD0 triangle count"), MeshDescription->Triangles().Num(), 1);
    }
    return true;
}
#endif

AGeometryStampActor::AGeometryStampActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bIsEditorOnlyActor = true;

#if WITH_EDITORONLY_DATA
    // PostEditMove provides a deliberately low-resolution drag preview instead.
    bRunConstructionScriptOnDrag = false;
#endif

    PreviewMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("PreviewMesh"));
    SetRootComponent(PreviewMesh);

    PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PreviewMesh->SetGenerateOverlapEvents(false);
    PreviewMesh->SetCastShadow(true);
}

void AGeometryStampActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (bAutoRebuild)
    {
        RebuildStamp();
    }
}

#if WITH_EDITOR
void AGeometryStampActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.GetPropertyName();
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AGeometryStampActor, QualityPreset))
    {
        ApplyQualityResolution();
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(AGeometryStampActor, GridResolution))
    {
        UpdateQualityFromResolution();
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(AGeometryStampActor, PreviewMaterial))
    {
        AutoConfigureHeightFromMaterial();
    }

    if (bAutoRebuild && !bIsRebuilding)
    {
        RebuildStamp();
    }
}

void AGeometryStampActor::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);

    if (!bAutoRebuild || bIsRebuilding)
    {
        return;
    }

    if (bFinished)
    {
        RebuildStamp();
        return;
    }

    if (!bLightweightMovePreview)
    {
        return;
    }

    const double CurrentTime = FPlatformTime::Seconds();
    if (LastMovePreviewTime >= 0.0 && CurrentTime - LastMovePreviewTime < MovePreviewUpdateInterval)
    {
        return;
    }

    LastMovePreviewTime = CurrentTime;
    RebuildStampInternal(MovePreviewResolution);
}

void AGeometryStampActor::PostEditUndo()
{
    Super::PostEditUndo();
    UpdateQualityFromResolution();
    RebuildStamp();
}
#endif

void AGeometryStampActor::ApplyPreset()
{
    if (!Preset)
    {
        return;
    }

    const FScopedTransaction Transaction(NSLOCTEXT("GeometryStamp", "ApplyPreset", "Apply Geometry Stamp Preset"));
    Modify();
    CopySettingsFromPreset(*Preset);
    UpdateQualityFromResolution();
    RebuildStamp();
}

void AGeometryStampActor::SaveCurrentAsPreset()
{
#if WITH_EDITOR
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();

    FString UniquePackageName;
    FString UniqueAssetName;
    AssetTools.CreateUniqueAssetName(
        TEXT("/Game/GeometryStamp/Presets/GS_Preset"),
        TEXT(""),
        UniquePackageName,
        UniqueAssetName);

    UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
    Factory->DataAssetClass = UGeometryStampPreset::StaticClass();

    UObject* CreatedAsset = AssetTools.CreateAssetWithDialog(
        UniqueAssetName,
        FPackageName::GetLongPackagePath(UniquePackageName),
        UGeometryStampPreset::StaticClass(),
        Factory,
        TEXT("GeometryStamp.SavePreset"),
        false);

    UGeometryStampPreset* NewPreset = Cast<UGeometryStampPreset>(CreatedAsset);
    if (!NewPreset)
    {
        return;
    }

    const FScopedTransaction Transaction(NSLOCTEXT("GeometryStamp", "SavePreset", "Save Geometry Stamp Preset"));
    Modify();
    NewPreset->SetFlags(RF_Transactional);
    NewPreset->Modify();
    CopySettingsToPreset(*NewPreset);
    NewPreset->MarkPackageDirty();
    Preset = NewPreset;
#endif
}

void AGeometryStampActor::ResetToDefaults()
{
    const AGeometryStampActor* Defaults = GetClass()->GetDefaultObject<AGeometryStampActor>();
    if (!Defaults)
    {
        return;
    }

    const FScopedTransaction Transaction(NSLOCTEXT("GeometryStamp", "ResetDefaults", "Reset Geometry Stamp to Defaults"));
    Modify();
    Preset = nullptr;
    ResetSettingsFrom(*Defaults);
    RebuildStamp();
}

void AGeometryStampActor::AutoConfigureFromMaterial()
{
    if (!PreviewMaterial)
    {
        return;
    }

    const FScopedTransaction Transaction(NSLOCTEXT("GeometryStamp", "AutoMaterial", "Auto Configure Geometry Stamp from Material"));
    Modify();
    if (AutoConfigureHeightFromMaterial())
    {
        RebuildStamp();
    }
}

bool AGeometryStampActor::AutoConfigureHeightFromMaterial()
{
    if (!PreviewMaterial)
    {
        const bool bChanged = HeightTexture != nullptr;
        HeightTexture = nullptr;
        return bChanged;
    }

    struct FHeightCandidate
    {
        TObjectPtr<UTexture2D> Texture;
        EGeometryStampHeightChannel Channel = EGeometryStampHeightChannel::Luminance;
        int32 Score = MIN_int32;
    };

    FHeightCandidate BestCandidate;
    TSet<UTexture2D*> SeenTextures;

    const auto ScoreName = [](const FString& LowerName, bool& bOutExplicitHeight)
    {
        bOutExplicitHeight = true;
        if (LowerName.Contains(TEXT("displacement")) || LowerName.Contains(TEXT("_disp")))
        {
            return 10000;
        }
        if (LowerName.Contains(TEXT("height")))
        {
            return 9000;
        }
        if (LowerName.Contains(TEXT("bump")) || LowerName.Contains(TEXT("parallax")))
        {
            return 8000;
        }

        bOutExplicitHeight = false;
        if (LowerName.Contains(TEXT("rough")))
        {
            return 600;
        }
        if (LowerName.Contains(TEXT("ambientocclusion")) || LowerName.Contains(TEXT("_ao")))
        {
            return 550;
        }
        if (LowerName.Contains(TEXT("basecolor")) || LowerName.Contains(TEXT("albedo")) || LowerName.Contains(TEXT("diffuse")))
        {
            return 500;
        }
        if (LowerName.Contains(TEXT("normal")))
        {
            return 100;
        }
        return 10;
    };

    const auto ConsiderTexture = [&BestCandidate, &SeenTextures, &ScoreName](UTexture* Texture, const FString& SourceName)
    {
        UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
        if (!Texture2D || SeenTextures.Contains(Texture2D) || !Texture2D->Source.IsValid())
        {
            return;
        }

        const ETextureSourceFormat SourceFormat = Texture2D->Source.GetFormat();
        if (SourceFormat != TSF_G8 && SourceFormat != TSF_BGRA8)
        {
            return;
        }

        SeenTextures.Add(Texture2D);
        const FString LowerName = (SourceName + TEXT("_") + Texture2D->GetName()).ToLower();
        bool bExplicitHeight = false;
        const int32 Score = ScoreName(LowerName, bExplicitHeight);
        if (Score <= BestCandidate.Score)
        {
            return;
        }

        EGeometryStampHeightChannel Channel = bExplicitHeight
            ? EGeometryStampHeightChannel::Red
            : EGeometryStampHeightChannel::Luminance;
        if (bExplicitHeight && (LowerName.Contains(TEXT("alpha")) || LowerName.EndsWith(TEXT("_a"))))
        {
            Channel = EGeometryStampHeightChannel::Alpha;
        }
        else if (bExplicitHeight && LowerName.EndsWith(TEXT("_g")))
        {
            Channel = EGeometryStampHeightChannel::Green;
        }
        else if (bExplicitHeight && LowerName.EndsWith(TEXT("_b")))
        {
            Channel = EGeometryStampHeightChannel::Blue;
        }

        BestCandidate.Texture = Texture2D;
        BestCandidate.Channel = Channel;
        BestCandidate.Score = Score;
    };

    TArray<FMaterialParameterInfo> ParameterInfos;
    TArray<FGuid> ParameterIds;
    PreviewMaterial->GetAllTextureParameterInfo(ParameterInfos, ParameterIds);
    for (const FMaterialParameterInfo& ParameterInfo : ParameterInfos)
    {
        UTexture* Texture = nullptr;
        if (PreviewMaterial->GetTextureParameterValue(ParameterInfo, Texture))
        {
            ConsiderTexture(Texture, ParameterInfo.Name.ToString());
        }
    }

    TArray<UTexture*> UsedTextures;
    PreviewMaterial->GetUsedTextures(
        UsedTextures,
        EMaterialQualityLevel::High,
        true,
        ERHIFeatureLevel::SM5,
        true);
    for (UTexture* Texture : UsedTextures)
    {
        ConsiderTexture(Texture, Texture ? Texture->GetName() : FString());
    }

    if (!BestCandidate.Texture)
    {
        const bool bChanged = HeightTexture != nullptr;
        HeightTexture = nullptr;
        return bChanged;
    }

    const bool bChanged = HeightTexture != BestCandidate.Texture || HeightChannel != BestCandidate.Channel;
    HeightTexture = BestCandidate.Texture;
    HeightChannel = BestCandidate.Channel;
    return bChanged;
}

void AGeometryStampActor::CopySettingsFromPreset(const UGeometryStampPreset& SourcePreset)
{
    PreviewMaterial = SourcePreset.Material;
    HeightTexture = SourcePreset.HeightTexture;
    HeightChannel = SourcePreset.HeightChannel;
    Shape = SourcePreset.Shape;
    Size = SourcePreset.Size;
    GridResolution = SourcePreset.GridResolution;
    Projection = SourcePreset.ProjectionMode;
    MaxSurfaceAngle = SourcePreset.MaxSurfaceAngle;
    TraceDistance = SourcePreset.TraceDistance;
    bTraceComplex = SourcePreset.bTraceComplex;
    HeightCenter = SourcePreset.HeightCenter;
    HeightMagnitude = SourcePreset.DisplacementStrength;
    HeightAdd = SourcePreset.SurfaceOffset;
    EdgeInset = SourcePreset.EdgeInset;
    EdgeFalloff = SourcePreset.EdgeFalloff;
    MaskHardness = SourcePreset.MaskHardness;
    HeightTiling = SourcePreset.HeightTiling;
    UVSize = SourcePreset.UVSize;
    bAutoRebuild = SourcePreset.bAutoRebuild;
    bLightweightMovePreview = SourcePreset.bLightweightMovePreview;
    MovePreviewResolution = SourcePreset.MovePreviewResolution;
    MovePreviewUpdateInterval = SourcePreset.MovePreviewUpdateInterval;
}

void AGeometryStampActor::CopySettingsToPreset(UGeometryStampPreset& TargetPreset) const
{
    TargetPreset.Material = PreviewMaterial;
    TargetPreset.HeightTexture = HeightTexture;
    TargetPreset.HeightChannel = HeightChannel;
    TargetPreset.Shape = Shape;
    TargetPreset.Size = Size;
    TargetPreset.GridResolution = GridResolution;
    TargetPreset.ProjectionMode = Projection;
    TargetPreset.MaxSurfaceAngle = MaxSurfaceAngle;
    TargetPreset.TraceDistance = TraceDistance;
    TargetPreset.bTraceComplex = bTraceComplex;
    TargetPreset.HeightCenter = HeightCenter;
    TargetPreset.DisplacementStrength = HeightMagnitude;
    TargetPreset.SurfaceOffset = HeightAdd;
    TargetPreset.EdgeInset = EdgeInset;
    TargetPreset.EdgeFalloff = EdgeFalloff;
    TargetPreset.MaskHardness = MaskHardness;
    TargetPreset.HeightTiling = HeightTiling;
    TargetPreset.UVSize = UVSize;
    TargetPreset.bAutoRebuild = bAutoRebuild;
    TargetPreset.bLightweightMovePreview = bLightweightMovePreview;
    TargetPreset.MovePreviewResolution = MovePreviewResolution;
    TargetPreset.MovePreviewUpdateInterval = MovePreviewUpdateInterval;
}

void AGeometryStampActor::ResetSettingsFrom(const AGeometryStampActor& Defaults)
{
    QualityPreset = Defaults.QualityPreset;
    GridResolution = Defaults.GridResolution;
    Shape = Defaults.Shape;
    Size = Defaults.Size;
    HeightTexture = Defaults.HeightTexture;
    HeightChannel = Defaults.HeightChannel;
    HeightCenter = Defaults.HeightCenter;
    HeightMagnitude = Defaults.HeightMagnitude;
    HeightAdd = Defaults.HeightAdd;
    EdgeInset = Defaults.EdgeInset;
    EdgeFalloff = Defaults.EdgeFalloff;
    MaskHardness = Defaults.MaskHardness;
    HeightTiling = Defaults.HeightTiling;
    PreviewMaterial = Defaults.PreviewMaterial;
    UVSize = Defaults.UVSize;
    Projection = Defaults.Projection;
    MaxSurfaceAngle = Defaults.MaxSurfaceAngle;
    bAutoRebuild = Defaults.bAutoRebuild;
    bLightweightMovePreview = Defaults.bLightweightMovePreview;
    MovePreviewResolution = Defaults.MovePreviewResolution;
    MovePreviewUpdateInterval = Defaults.MovePreviewUpdateInterval;
    TraceDistance = Defaults.TraceDistance;
    bTraceComplex = Defaults.bTraceComplex;
}

void AGeometryStampActor::UpdateQualityFromResolution()
{
    switch (GridResolution)
    {
    case 16:
        QualityPreset = EGeometryStampQuality::Draft;
        break;
    case 64:
        QualityPreset = EGeometryStampQuality::Preview;
        break;
    case 128:
        QualityPreset = EGeometryStampQuality::High;
        break;
    case 256:
        QualityPreset = EGeometryStampQuality::Bake;
        break;
    default:
        QualityPreset = EGeometryStampQuality::Custom;
        break;
    }
}

void AGeometryStampActor::ApplyQualityResolution()
{
    switch (QualityPreset)
    {
    case EGeometryStampQuality::Draft:
        GridResolution = 16;
        break;
    case EGeometryStampQuality::Preview:
        GridResolution = 64;
        break;
    case EGeometryStampQuality::High:
        GridResolution = 128;
        break;
    case EGeometryStampQuality::Bake:
        GridResolution = 256;
        break;
    case EGeometryStampQuality::Custom:
    default:
        break;
    }
}

void AGeometryStampActor::RebuildStamp()
{
    RebuildStampInternal(GridResolution);
}

void AGeometryStampActor::RebuildStampInternal(int32 Resolution)
{
    if (bIsRebuilding || !PreviewMesh || !GetWorld())
    {
        return;
    }

    TGuardValue<bool> RebuildGuard(bIsRebuilding, true);

    FDynamicMesh3 GeneratedMesh(true, true, true, false);
    BuildProjectedMesh(GeneratedMesh, Resolution);

    PreviewMesh->GetDynamicMesh()->SetMesh(MoveTemp(GeneratedMesh));
    PreviewMesh->SetMaterial(0, PreviewMaterial);
    PreviewMesh->NotifyMeshUpdated();
}

void AGeometryStampActor::ClearStamp()
{
    if (!PreviewMesh)
    {
        return;
    }

    PreviewMesh->GetDynamicMesh()->Reset();
    PreviewMesh->NotifyMeshUpdated();
}

#if WITH_EDITOR
bool AGeometryStampActor::HasBakeableMesh() const
{
    bool bHasTriangles = false;
    if (PreviewMesh)
    {
        PreviewMesh->ProcessMesh([&bHasTriangles](const FDynamicMesh3& Mesh)
        {
            bHasTriangles = Mesh.TriangleCount() > 0;
        });
    }
    return bHasTriangles;
}

bool AGeometryStampActor::BakeToStaticMesh(
    const FString& AssetFolder,
    const FString& AssetName,
    bool bEnableNanite,
    bool bReplaceActor,
    FText& OutError)
{
    OutError = FText::GetEmpty();

    FString CleanFolder = AssetFolder.TrimStartAndEnd().Replace(TEXT("\\"), TEXT("/"));
    while (CleanFolder.EndsWith(TEXT("/")))
    {
        CleanFolder.LeftChopInline(1);
    }

    const FString CleanAssetName = ObjectTools::SanitizeObjectName(AssetName.TrimStartAndEnd());
    if (CleanFolder.IsEmpty() || CleanAssetName.IsEmpty())
    {
        OutError = NSLOCTEXT("GeometryStamp", "BakeMissingPath", "Enter both an asset folder and asset name.");
        return false;
    }

    const FString PackageName = CleanFolder / CleanAssetName;
    FText PackageError;
    if (!FPackageName::IsValidLongPackageName(PackageName, false, &PackageError))
    {
        OutError = FText::Format(
            NSLOCTEXT("GeometryStamp", "BakeInvalidPath", "Invalid asset path: {0}"),
            PackageError);
        return false;
    }

    if (!PackageName.StartsWith(TEXT("/Game/")))
    {
        OutError = NSLOCTEXT("GeometryStamp", "BakeOutsideGame", "Bake assets must be created inside /Game.");
        return false;
    }

    if (FPackageName::DoesPackageExist(PackageName) || FindPackage(nullptr, *PackageName))
    {
        OutError = FText::Format(
            NSLOCTEXT("GeometryStamp", "BakeAssetExists", "{0} already exists. Choose a new name; Geometry Stamp never overwrites bake assets."),
            FText::FromString(PackageName));
        return false;
    }

    if (!HasBakeableMesh())
    {
        OutError = NSLOCTEXT("GeometryStamp", "BakeEmptyMesh", "The stamp preview is empty. Rebuild the stamp before baking.");
        return false;
    }

    UPackage* Package = CreatePackage(*PackageName);
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(
        Package,
        *CleanAssetName,
        RF_Public | RF_Standalone | RF_Transactional);

    bool bBuilt = false;
    PreviewMesh->ProcessMesh([this, StaticMesh, bEnableNanite, &bBuilt](const FDynamicMesh3& Mesh)
    {
        bBuilt = BuildStaticMeshFromDynamicMesh(*StaticMesh, Mesh, PreviewMaterial, bEnableNanite);
    });

    if (!bBuilt)
    {
        OutError = NSLOCTEXT("GeometryStamp", "BakeConversionFailed", "Could not convert the preview mesh to a Static Mesh.");
        return false;
    }

    FAssetRegistryModule::AssetCreated(StaticMesh);
    Package->MarkPackageDirty();

    if (bReplaceActor)
    {
        UWorld* World = GetWorld();
        if (!World)
        {
            OutError = NSLOCTEXT("GeometryStamp", "BakeMissingWorld", "The stamp is not placed in an editor world.");
            return false;
        }

        const FScopedTransaction Transaction(NSLOCTEXT("GeometryStamp", "BakeReplaceActor", "Replace Geometry Stamp with Static Mesh"));
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.OverrideLevel = GetLevel();
        SpawnParameters.ObjectFlags |= RF_Transactional;

        AStaticMeshActor* BakedActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(),
            GetActorTransform(),
            SpawnParameters);
        if (!BakedActor)
        {
            OutError = NSLOCTEXT("GeometryStamp", "BakeSpawnFailed", "The asset was created, but the Static Mesh Actor could not be placed.");
            return false;
        }

        BakedActor->SetActorLabel(GetActorLabel() + TEXT("_Baked"));
        BakedActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Static);
        BakedActor->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);

        Modify();
        World->EditorDestroyActor(this, true);
        GEditor->SelectNone(false, true, false);
        GEditor->SelectActor(BakedActor, true, true);
    }

    TArray<UObject*> ObjectsToSync;
    ObjectsToSync.Add(StaticMesh);
    GEditor->SyncBrowserToObjects(ObjectsToSync);
    return true;
}
#endif

void AGeometryStampActor::BuildProjectedMesh(FDynamicMesh3& Mesh, int32 RequestedResolution)
{
    const int32 Resolution = FMath::Clamp(RequestedResolution, 2, 512);
    const int32 VerticesPerSide = Resolution + 1;
    const FTransform ActorTransform = GetActorTransform();
    const FVector TraceDirection = Projection == EGeometryStampProjection::ActorDown
        ? -GetActorUpVector()
        : FVector::DownVector;

    TArray<int32> VertexIDs;
    VertexIDs.Init(INDEX_NONE, VerticesPerSide * VerticesPerSide);
    TArray<FVector> WorldPositions;
    WorldPositions.Init(FVector::ZeroVector, VerticesPerSide * VerticesPerSide);

    FGeometryStampHeightSampler HeightSampler;
    HeightSampler.Initialize(HeightTexture, HeightChannel);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GeometryStampProjection), bTraceComplex, this);
    QueryParams.bReturnPhysicalMaterial = false;

    for (int32 Y = 0; Y < VerticesPerSide; ++Y)
    {
        for (int32 X = 0; X < VerticesPerSide; ++X)
        {
            const FVector2D UV(
                static_cast<double>(X) / Resolution,
                static_cast<double>(Y) / Resolution);
            const FVector2D NormalizedPosition = UV * 2.0 - FVector2D(1.0, 1.0);
            const float ShapeDistance = CalculateShapeDistance(NormalizedPosition);
            const FVector2D ShapePosition = MapShapePosition(NormalizedPosition);

            const FVector LocalPlanePosition(
                ShapePosition.X * Size.X * 0.5,
                ShapePosition.Y * Size.Y * 0.5,
                0.0);
            const FVector WorldPlanePosition = ActorTransform.TransformPosition(LocalPlanePosition);
            const FVector TraceStart = WorldPlanePosition - TraceDirection * TraceDistance;
            const FVector TraceEnd = WorldPlanePosition + TraceDirection * TraceDistance;

            FHitResult Hit;
            if (!GetWorld()->LineTraceSingleByChannel(
                    Hit,
                    TraceStart,
                    TraceEnd,
                    ECC_Visibility,
                    QueryParams))
            {
                continue;
            }

            if (!IsSurfaceAngleAllowed(TraceDirection, Hit.ImpactNormal, MaxSurfaceAngle))
            {
                continue;
            }

            float HeightValue = HeightCenter;
            HeightSampler.Sample(UV, HeightTiling, HeightValue);

            const float EdgeMask = CalculateEdgeMask(ShapeDistance);
            const double Displacement =
                ((static_cast<double>(HeightValue) - HeightCenter) * HeightMagnitude + HeightAdd) * EdgeMask
                - EdgeInset * (1.0 - EdgeMask);
            const FVector WorldVertexPosition = Hit.ImpactPoint + Hit.ImpactNormal * Displacement;
            const FVector LocalVertexPosition = ActorTransform.InverseTransformPosition(WorldVertexPosition);

            const int32 VertexID = Mesh.AppendVertex(FVector3d(LocalVertexPosition));
            Mesh.SetVertexUV(VertexID, FVector2f(
                static_cast<float>(LocalPlanePosition.X / UVSize),
                static_cast<float>(LocalPlanePosition.Y / UVSize)));
            const int32 GridIndex = Y * VerticesPerSide + X;
            VertexIDs[GridIndex] = VertexID;
            WorldPositions[GridIndex] = WorldVertexPosition;
        }
    }

    for (int32 Y = 0; Y < Resolution; ++Y)
    {
        for (int32 X = 0; X < Resolution; ++X)
        {
            const int32 V00 = VertexIDs[Y * VerticesPerSide + X];
            const int32 V10 = VertexIDs[Y * VerticesPerSide + X + 1];
            const int32 V01 = VertexIDs[(Y + 1) * VerticesPerSide + X];
            const int32 V11 = VertexIDs[(Y + 1) * VerticesPerSide + X + 1];

            if (V00 == INDEX_NONE || V10 == INDEX_NONE || V01 == INDEX_NONE || V11 == INDEX_NONE)
            {
                continue;
            }

            if (IsTriangleSurfaceAngleAllowed(
                    TraceDirection,
                    WorldPositions[Y * VerticesPerSide + X],
                    WorldPositions[(Y + 1) * VerticesPerSide + X + 1],
                    WorldPositions[Y * VerticesPerSide + X + 1],
                    MaxSurfaceAngle))
            {
                Mesh.AppendTriangle(V00, V11, V10);
            }
            if (IsTriangleSurfaceAngleAllowed(
                    TraceDirection,
                    WorldPositions[Y * VerticesPerSide + X],
                    WorldPositions[(Y + 1) * VerticesPerSide + X],
                    WorldPositions[(Y + 1) * VerticesPerSide + X + 1],
                    MaxSurfaceAngle))
            {
                Mesh.AppendTriangle(V00, V01, V11);
            }
        }
    }

    if (Mesh.TriangleCount() > 0)
    {
        FMeshNormals::QuickComputeVertexNormals(Mesh, false);
    }
}

FVector2D AGeometryStampActor::MapShapePosition(const FVector2D& NormalizedPosition) const
{
    if (Shape == EGeometryStampShape::Rectangle)
    {
        return NormalizedPosition;
    }

    // Elliptical square-to-disc mapping keeps the complete quad grid while
    // turning the square boundary into a smooth circle/ellipse.
    const double XSquared = FMath::Square(NormalizedPosition.X);
    const double YSquared = FMath::Square(NormalizedPosition.Y);
    return FVector2D(
        NormalizedPosition.X * FMath::Sqrt(FMath::Max(0.0, 1.0 - YSquared * 0.5)),
        NormalizedPosition.Y * FMath::Sqrt(FMath::Max(0.0, 1.0 - XSquared * 0.5)));
}

float AGeometryStampActor::CalculateShapeDistance(const FVector2D& NormalizedPosition) const
{
    return static_cast<float>(FMath::Max(FMath::Abs(NormalizedPosition.X), FMath::Abs(NormalizedPosition.Y)));
}

float AGeometryStampActor::CalculateEdgeMask(float ShapeDistance) const
{
    const float SafeFalloff = FMath::Max(EdgeFalloff, 0.001f);
    const float LinearMask = FMath::Clamp((1.0f - ShapeDistance) / SafeFalloff, 0.0f, 1.0f);
    const float SmoothMask = LinearMask * LinearMask * (3.0f - 2.0f * LinearMask);
    return FMath::Pow(SmoothMask, FMath::Max(MaskHardness, 0.01f));
}
