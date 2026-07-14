#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeometryStampActor.generated.h"

class UDynamicMeshComponent;
class UMaterialInterface;
class UTexture2D;

namespace UE::Geometry
{
    class FDynamicMesh3;
}

UENUM(BlueprintType)
enum class EGeometryStampShape : uint8
{
    Circle UMETA(DisplayName = "Circle"),
    Rectangle UMETA(DisplayName = "Rectangle")
};

UENUM(BlueprintType)
enum class EGeometryStampProjection : uint8
{
    WorldDown UMETA(DisplayName = "World Down"),
    ActorDown UMETA(DisplayName = "Actor Down")
};

UENUM(BlueprintType)
enum class EGeometryStampHeightChannel : uint8
{
    Red UMETA(DisplayName = "Red"),
    Green UMETA(DisplayName = "Green"),
    Blue UMETA(DisplayName = "Blue"),
    Alpha UMETA(DisplayName = "Alpha"),
    Luminance UMETA(DisplayName = "Luminance")
};

/**
 * Editor-only geometry stamp that creates a projected Dynamic Mesh preview.
 * The generated mesh is never intended to ship directly; a later milestone
 * converts the preview to a Nanite-ready Static Mesh asset.
 */
UCLASS(BlueprintType, Blueprintable, HideCategories = (Replication, Networking, Input))
class GEOMETRYSTAMP_API AGeometryStampActor : public AActor
{
    GENERATED_BODY()

public:
    AGeometryStampActor();

    virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Geometry Stamp")
    void RebuildStamp();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Geometry Stamp")
    void ClearStamp();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Geometry Stamp")
    TObjectPtr<UDynamicMeshComponent> PreviewMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Shape")
    EGeometryStampShape Shape = EGeometryStampShape::Circle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Shape", meta = (ClampMin = "1.0", UIMin = "10.0", Units = "cm"))
    FVector2D Size = FVector2D(1000.0, 1000.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Shape", meta = (ClampMin = "2", ClampMax = "512", UIMin = "8", UIMax = "256"))
    int32 GridResolution = 64;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Projection")
    EGeometryStampProjection Projection = EGeometryStampProjection::WorldDown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Projection", meta = (ClampMin = "1.0", UIMin = "100.0", Units = "cm"))
    double TraceDistance = 100000.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Projection")
    bool bTraceComplex = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height")
    TObjectPtr<UTexture2D> HeightTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height")
    EGeometryStampHeightChannel HeightChannel = EGeometryStampHeightChannel::Red;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float HeightCenter = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height", meta = (UIMin = "-1000.0", UIMax = "1000.0", Units = "cm"))
    double HeightMagnitude = 100.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height", meta = (UIMin = "-100.0", UIMax = "100.0", Units = "cm"))
    double HeightAdd = 0.5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height", meta = (ClampMin = "0.001", ClampMax = "1.0", UIMin = "0.01", UIMax = "1.0"))
    float EdgeFalloff = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height", meta = (ClampMin = "0.01", ClampMax = "16.0", UIMin = "0.1", UIMax = "8.0"))
    float MaskHardness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Height", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "8.0"))
    FVector2D HeightTiling = FVector2D(1.0, 1.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Material")
    TObjectPtr<UMaterialInterface> PreviewMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Material", meta = (ClampMin = "0.01", UIMin = "1.0", Units = "cm"))
    double UVSize = 100.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry Stamp|Editor")
    bool bAutoRebuild = true;

private:
    bool bIsRebuilding = false;

    void BuildProjectedMesh(UE::Geometry::FDynamicMesh3& Mesh);
    float CalculateShapeDistance(const FVector2D& NormalizedPosition) const;
    float CalculateEdgeMask(float ShapeDistance) const;
};
