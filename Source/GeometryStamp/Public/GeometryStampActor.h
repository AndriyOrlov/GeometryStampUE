#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Actor.h"
#include "GeometryStampTypes.h"
#include "GeometryStampActor.generated.h"

class UDynamicMeshComponent;
class UGeometryStampPreset;
class UMaterialInterface;
class UTexture2D;

namespace UE::Geometry
{
    class FDynamicMesh3;
}

/**
 * Editor-only geometry stamp that creates a projected Dynamic Mesh preview.
 * The generated mesh is an editable preview that can be baked to a
 * Nanite-ready Static Mesh asset from the Details panel.
 */
UCLASS(
    BlueprintType,
    Blueprintable,
    ClassGroup = (GeometryStamp),
    AutoExpandCategories = ("Geometry Stamp"),
    meta = (DisplayName = "Geometry Stamp"),
    HideCategories = (Replication, Networking, Input))
class GEOMETRYSTAMP_API AGeometryStampActor : public AActor
{
    GENERATED_BODY()

public:
    AGeometryStampActor();

    virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void PostEditMove(bool bFinished) override;
    virtual void PostEditUndo() override;
#endif

    UFUNCTION(BlueprintCallable, Category = "Geometry Stamp")
    void ApplyPreset();

    UFUNCTION(BlueprintCallable, Category = "Geometry Stamp")
    void SaveCurrentAsPreset();

    UFUNCTION(BlueprintCallable, Category = "Geometry Stamp")
    void ResetToDefaults();

    UFUNCTION(BlueprintCallable, Category = "Geometry Stamp")
    void AutoConfigureFromMaterial();

    UFUNCTION(BlueprintCallable, Category = "Geometry Stamp")
    void RebuildStamp();

    UFUNCTION(BlueprintCallable, Category = "Geometry Stamp")
    void ClearStamp();

    bool HasPreset() const { return Preset != nullptr; }

#if WITH_EDITOR
    bool HasBakeableMesh() const;
    bool BakeToStaticMesh(
        const FString& AssetFolder,
        const FString& AssetName,
        EGeometryStampQuality BakeQuality,
        bool bEnableNanite,
        bool bReplaceActor,
        FText& OutError);
#endif

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Geometry Stamp")
    TObjectPtr<UDynamicMeshComponent> PreviewMesh;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Geometry Stamp",
        meta = (
            ForceShowPluginContent,
            ToolTip = "Reusable scene-independent stamp settings. Apply Preset copies the asset values to this actor."))
    TObjectPtr<UGeometryStampPreset> Preset;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Geometry Stamp|Quality",
        meta = (DisplayName = "Quality Preset", ToolTip = "Changes only Grid Resolution."))
    EGeometryStampQuality QualityPreset = EGeometryStampQuality::Preview;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Geometry Stamp|Quality",
        meta = (ClampMin = "2", ClampMax = "512", UIMin = "8", UIMax = "256", ToolTip = "Geometry subdivisions per side. Higher values perform more projection traces."))
    int32 GridResolution = 64;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Shape")
    EGeometryStampShape Shape = EGeometryStampShape::Circle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Shape", meta = (ClampMin = "1.0", UIMin = "10.0", Units = "cm"))
    FVector2D Size = FVector2D(1000.0, 1000.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height / Displacement")
    TObjectPtr<UTexture2D> HeightTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height / Displacement")
    EGeometryStampHeightChannel HeightChannel = EGeometryStampHeightChannel::Red;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height / Displacement", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float HeightCenter = 0.5f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Geometry Stamp|Height / Displacement",
        meta = (
            DisplayName = "Displacement Strength",
            ToolTip = "Displacement range in centimeters. This is independent of the actor height.",
            UIMin = "-1000.0",
            UIMax = "1000.0",
            Units = "cm"))
    double HeightMagnitude = 100.0;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Geometry Stamp|Height / Displacement",
        meta = (
            DisplayName = "Surface Offset",
            ToolTip = "Small normal offset used to prevent z-fighting with the projected surface.",
            UIMin = "-100.0",
            UIMax = "100.0",
            Units = "cm"))
    double HeightAdd = 0.5;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Geometry Stamp|Height / Displacement",
        meta = (
            DisplayName = "Edge Inset",
            ToolTip = "Pushes the outer edge into the projected surface so the stamp transitions without a visible gap.",
            ClampMin = "0.0",
            UIMin = "0.0",
            UIMax = "20.0",
            Units = "cm"))
    double EdgeInset = 2.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height / Displacement", meta = (ClampMin = "0.001", ClampMax = "1.0", UIMin = "0.01", UIMax = "1.0"))
    float EdgeFalloff = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height / Displacement", meta = (ClampMin = "0.01", ClampMax = "16.0", UIMin = "0.1", UIMax = "8.0"))
    float MaskHardness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height / Displacement", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "8.0"))
    FVector2D HeightTiling = FVector2D(1.0, 1.0);

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Geometry Stamp|Height / Displacement",
        meta = (
            DisplayName = "Height Profile",
            XAxisName = "Input Height",
            YAxisName = "Output Height",
            ToolTip = "Remaps sampled height from 0..1 to 0..1 before displacement. The default curve is linear from (0,0) to (1,1)."))
    FRuntimeFloatCurve HeightProfile;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Geometry Stamp|Material / UV",
        meta = (
            DisplayName = "Material",
            ToolTip = "Material shown on the stamp. Assigning it automatically detects Displacement or Height, then falls back to luminance from an available texture."))
    TObjectPtr<UMaterialInterface> PreviewMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Material / UV", meta = (ClampMin = "0.01", UIMin = "1.0", Units = "cm"))
    double UVSize = 100.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Projection")
    EGeometryStampProjection Projection = EGeometryStampProjection::WorldDown;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Geometry Stamp|Projection",
        meta = (
            DisplayName = "Max Surface Angle",
            ToolTip = "Reject projected geometry on surfaces steeper than this angle relative to the projection plane. Use 85 degrees to prevent spill onto walls.",
            ClampMin = "0.0",
            ClampMax = "90.0",
            UIMin = "0.0",
            UIMax = "90.0",
            Units = "deg"))
    double MaxSurfaceAngle = 85.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Preview")
    bool bAutoRebuild = true;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Geometry Stamp|Preview",
        meta = (EditCondition = "bAutoRebuild"))
    bool bLightweightMovePreview = true;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Geometry Stamp|Preview",
        meta = (
            ClampMin = "2",
            ClampMax = "64",
            UIMin = "4",
            UIMax = "32",
            EditCondition = "bAutoRebuild && bLightweightMovePreview"))
    int32 MovePreviewResolution = 16;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Geometry Stamp|Preview",
        meta = (
            ClampMin = "0.0",
            ClampMax = "1.0",
            UIMin = "0.0",
            UIMax = "0.25",
            Units = "s",
            EditCondition = "bAutoRebuild && bLightweightMovePreview"))
    double MovePreviewUpdateInterval = 0.05;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Advanced", meta = (ClampMin = "1.0", UIMin = "100.0", Units = "cm"))
    double TraceDistance = 100000.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Advanced")
    bool bTraceComplex = true;

private:
    bool bIsRebuilding = false;
    double LastMovePreviewTime = -1.0;

    void CopySettingsFromPreset(const UGeometryStampPreset& SourcePreset);
    void CopySettingsToPreset(UGeometryStampPreset& TargetPreset) const;
    void ResetSettingsFrom(const AGeometryStampActor& Defaults);
    bool AutoConfigureHeightFromMaterial();
    void UpdateQualityFromResolution();
    void ApplyQualityResolution();
    void RebuildStampInternal(int32 Resolution);
    void BuildProjectedMesh(UE::Geometry::FDynamicMesh3& Mesh, int32 Resolution);
    FVector2D MapShapePosition(const FVector2D& NormalizedPosition) const;
    float CalculateShapeDistance(const FVector2D& NormalizedPosition) const;
    float CalculateEdgeMask(float ShapeDistance) const;
};
