#include "GeometryStampPreset.h"

UGeometryStampPreset::UGeometryStampPreset()
{
    HeightProfile.EditorCurveData.AddKey(0.0f, 0.0f);
    HeightProfile.EditorCurveData.AddKey(1.0f, 1.0f);
}
