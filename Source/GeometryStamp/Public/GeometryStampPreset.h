#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "GeometryStampTypes.h"
#include "GeometryStampPreset.generated.h"

class UMaterialInterface;
class UTexture2D;

/** Reusable, scene-independent settings for a Geometry Stamp actor. */
UCLASS(BlueprintType, meta = (DisplayName = "Geometry Stamp Preset"))
class GEOMETRYSTAMP_API UGeometryStampPreset : public UDataAsset
{
    GENERATED_BODY()

public:
    UGeometryStampPreset();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material / UV")
    TObjectPtr<UMaterialInterface> Material;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Height / Displacement")
    TObjectPtr<UTexture2D> HeightTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Height / Displacement")
    EGeometryStampHeightChannel HeightChannel = EGeometryStampHeightChannel::Red;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape")
    EGeometryStampShape Shape = EGeometryStampShape::Circle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape", meta = (ClampMin = "1.0", Units = "cm"))
    FVector2D Size = FVector2D(1000.0, 1000.0);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "2", ClampMax = "512"))
    int32 GridResolution = 64;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projection")
    EGeometryStampProjection ProjectionMode = EGeometryStampProjection::WorldDown;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projection", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "deg"))
    double MaxSurfaceAngle = 85.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projection", meta = (ClampMin = "1.0", Units = "cm"))
    double TraceDistance = 100000.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projection")
    bool bTraceComplex = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Height / Displacement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HeightCenter = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Height / Displacement", meta = (DisplayName = "Displacement Strength", Units = "cm"))
    double DisplacementStrength = 100.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Height / Displacement", meta = (Units = "cm"))
    double SurfaceOffset = 0.5;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Height / Displacement", meta = (ClampMin = "0.0", Units = "cm"))
    double EdgeInset = 2.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Height / Displacement", meta = (ClampMin = "0.001", ClampMax = "1.0"))
    float EdgeFalloff = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Height / Displacement", meta = (ClampMin = "0.01", ClampMax = "16.0"))
    float MaskHardness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Height / Displacement", meta = (ClampMin = "0.01"))
    FVector2D HeightTiling = FVector2D(1.0, 1.0);

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Height / Displacement",
        meta = (
            DisplayName = "Height Profile",
            XAxisName = "Input Height",
            YAxisName = "Output Height",
            ToolTip = "Remaps sampled height from 0..1 to 0..1 before displacement."))
    FRuntimeFloatCurve HeightProfile;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material / UV", meta = (ClampMin = "0.01", Units = "cm"))
    double UVSize = 100.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview")
    bool bAutoRebuild = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview")
    bool bLightweightMovePreview = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview", meta = (ClampMin = "2", ClampMax = "64"))
    int32 MovePreviewResolution = 16;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview", meta = (ClampMin = "0.0", ClampMax = "1.0", Units = "s"))
    double MovePreviewUpdateInterval = 0.05;
};
