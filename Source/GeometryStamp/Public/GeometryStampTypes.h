#pragma once

#include "CoreMinimal.h"
#include "GeometryStampTypes.generated.h"

UENUM(BlueprintType)
enum class EGeometryStampShape : uint8
{
    Circle UMETA(DisplayName = "Circle"),
    // Keep the serialized enum name for compatibility with existing 0.3 actors.
    Rectangle UMETA(DisplayName = "Square")
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

UENUM(BlueprintType)
enum class EGeometryStampQuality : uint8
{
    Draft UMETA(DisplayName = "Draft (16)"),
    Preview UMETA(DisplayName = "Preview (64)"),
    High UMETA(DisplayName = "High (128)"),
    Bake UMETA(DisplayName = "Bake (256)"),
    Custom UMETA(DisplayName = "Custom")
};
