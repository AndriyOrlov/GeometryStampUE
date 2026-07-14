#include "GeometryStampActor.h"

#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshNormals.h"
#include "DynamicMesh/UDynamicMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"

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
}

AGeometryStampActor::AGeometryStampActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bIsEditorOnlyActor = true;

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

    if (bAutoRebuild && !bIsRebuilding)
    {
        RebuildStamp();
    }
}
#endif

void AGeometryStampActor::RebuildStamp()
{
    if (bIsRebuilding || !PreviewMesh || !GetWorld())
    {
        return;
    }

    TGuardValue<bool> RebuildGuard(bIsRebuilding, true);

    FDynamicMesh3 GeneratedMesh(true, true, true, false);
    BuildProjectedMesh(GeneratedMesh);

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

void AGeometryStampActor::BuildProjectedMesh(FDynamicMesh3& Mesh)
{
    const int32 Resolution = FMath::Clamp(GridResolution, 2, 512);
    const int32 VerticesPerSide = Resolution + 1;
    const FTransform ActorTransform = GetActorTransform();
    const FVector TraceDirection = Projection == EGeometryStampProjection::ActorDown
        ? -GetActorUpVector()
        : FVector::DownVector;

    TArray<int32> VertexIDs;
    VertexIDs.Init(INDEX_NONE, VerticesPerSide * VerticesPerSide);

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

            if (ShapeDistance > 1.0f)
            {
                continue;
            }

            const FVector LocalPlanePosition(
                NormalizedPosition.X * Size.X * 0.5,
                NormalizedPosition.Y * Size.Y * 0.5,
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

            float HeightValue = HeightCenter;
            HeightSampler.Sample(UV, HeightTiling, HeightValue);

            const float EdgeMask = CalculateEdgeMask(ShapeDistance);
            const double Displacement =
                ((static_cast<double>(HeightValue) - HeightCenter) * HeightMagnitude + HeightAdd) * EdgeMask;
            const FVector WorldVertexPosition = Hit.ImpactPoint + Hit.ImpactNormal * Displacement;
            const FVector LocalVertexPosition = ActorTransform.InverseTransformPosition(WorldVertexPosition);

            const int32 VertexID = Mesh.AppendVertex(FVector3d(LocalVertexPosition));
            Mesh.SetVertexUV(VertexID, FVector2f(
                static_cast<float>(LocalPlanePosition.X / UVSize),
                static_cast<float>(LocalPlanePosition.Y / UVSize)));
            VertexIDs[Y * VerticesPerSide + X] = VertexID;
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

            Mesh.AppendTriangle(V00, V10, V11);
            Mesh.AppendTriangle(V00, V11, V01);
        }
    }

    if (Mesh.TriangleCount() > 0)
    {
        FMeshNormals::QuickComputeVertexNormals(Mesh, false);
    }
}

float AGeometryStampActor::CalculateShapeDistance(const FVector2D& NormalizedPosition) const
{
    if (Shape == EGeometryStampShape::Rectangle)
    {
        return static_cast<float>(FMath::Max(FMath::Abs(NormalizedPosition.X), FMath::Abs(NormalizedPosition.Y)));
    }

    return static_cast<float>(NormalizedPosition.Size());
}

float AGeometryStampActor::CalculateEdgeMask(float ShapeDistance) const
{
    const float SafeFalloff = FMath::Max(EdgeFalloff, 0.001f);
    const float LinearMask = FMath::Clamp((1.0f - ShapeDistance) / SafeFalloff, 0.0f, 1.0f);
    const float SmoothMask = LinearMask * LinearMask * (3.0f - 2.0f * LinearMask);
    return FMath::Pow(SmoothMask, FMath::Max(MaskHardness, 0.01f));
}
