#include "GeometryStampActorDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

TSharedRef<IDetailCustomization> FGeometryStampActorDetails::MakeInstance()
{
    return MakeShared<FGeometryStampActorDetails>();
}

void FGeometryStampActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    IDetailCategoryBuilder& GeometryStampCategory = DetailBuilder.EditCategory(
        TEXT("Geometry Stamp"),
        FText::FromString(TEXT("Geometry Stamp")),
        ECategoryPriority::Important);

    // Keep the complete artist-facing tool block above generic Actor categories.
    GeometryStampCategory.SetSortOrder(0);
    DetailBuilder.EditCategory(TEXT("Transform")).SetSortOrder(1);
}
