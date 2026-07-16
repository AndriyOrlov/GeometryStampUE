#include "GeometryStampActorDetails.h"

#include "GeometryStampActor.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "GeometryStampActorDetails"

TSharedRef<IDetailCustomization> FGeometryStampActorDetails::MakeInstance()
{
    return MakeShared<FGeometryStampActorDetails>();
}

void FGeometryStampActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    SelectedActors.Reset();
    TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
    DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);
    for (const TWeakObjectPtr<UObject>& Object : CustomizedObjects)
    {
        if (AGeometryStampActor* Actor = Cast<AGeometryStampActor>(Object.Get()))
        {
            SelectedActors.Add(Actor);
        }
    }

    IDetailCategoryBuilder& GeometryStampCategory = DetailBuilder.EditCategory(
        TEXT("Geometry Stamp"),
        LOCTEXT("GeometryStampCategory", "Geometry Stamp"),
        ECategoryPriority::Important);

    GeometryStampCategory.SetSortOrder(0);
    DetailBuilder.EditCategory(TEXT("Transform")).SetSortOrder(1);

    const auto AddTopProperty = [&DetailBuilder, &GeometryStampCategory](const FName PropertyName)
    {
        const TSharedRef<IPropertyHandle> Property = DetailBuilder.GetProperty(PropertyName, AGeometryStampActor::StaticClass());
        DetailBuilder.HideProperty(Property);
        GeometryStampCategory.AddProperty(Property);
    };

    AddTopProperty(TEXT("Preset"));
    AddTopProperty(TEXT("QualityPreset"));
    AddTopProperty(TEXT("GridResolution"));

    GeometryStampCategory.AddCustomRow(LOCTEXT("PresetActionsFilter", "Preset Actions"))
    .WholeRowContent()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 2.0f, 0.0f)
        [
            SNew(SButton)
            .Text(LOCTEXT("ApplyPreset", "Apply Preset"))
            .ToolTipText(LOCTEXT("ApplyPresetTooltip", "Copy the selected preset settings to this actor and rebuild. Supports Undo/Redo."))
            .IsEnabled(this, &FGeometryStampActorDetails::CanApplyPreset)
            .OnClicked(this, &FGeometryStampActorDetails::ApplyPreset)
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.5f)
        .Padding(2.0f, 0.0f)
        [
            SNew(SButton)
            .Text(LOCTEXT("SavePreset", "Save Current as Preset"))
            .ToolTipText(LOCTEXT("SavePresetTooltip", "Create a new preset asset from the current actor. Existing preset assets are never overwritten."))
            .IsEnabled(this, &FGeometryStampActorDetails::CanSavePreset)
            .OnClicked(this, &FGeometryStampActorDetails::SaveCurrentAsPreset)
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.2f)
        .Padding(2.0f, 0.0f, 0.0f, 0.0f)
        [
            SNew(SButton)
            .Text(LOCTEXT("ResetDefaults", "Reset to Defaults"))
            .ToolTipText(LOCTEXT("ResetDefaultsTooltip", "Restore the class defaults without changing actor transform. Supports Undo/Redo."))
            .OnClicked(this, &FGeometryStampActorDetails::ResetToDefaults)
        ]
    ];

    GeometryStampCategory.AddCustomRow(LOCTEXT("PreviewActionsFilter", "Preview Actions"))
    .WholeRowContent()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.2f)
        .Padding(0.0f, 0.0f, 2.0f, 0.0f)
        [
            SNew(SButton)
            .Text(LOCTEXT("AutoMaterial", "Auto from Material"))
            .ToolTipText(LOCTEXT("AutoMaterialTooltip", "Find Displacement or Height textures in the material. If neither exists, use luminance from the best available texture map. No texture asset is overwritten."))
            .OnClicked(this, &FGeometryStampActorDetails::AutoConfigureFromMaterial)
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(2.0f, 0.0f)
        [
            SNew(SButton)
            .Text(LOCTEXT("RebuildStamp", "Rebuild Stamp"))
            .ToolTipText(LOCTEXT("RebuildStampTooltip", "Rebuild the full-resolution Dynamic Mesh preview."))
            .OnClicked(this, &FGeometryStampActorDetails::RebuildStamp)
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(2.0f, 0.0f, 0.0f, 0.0f)
        [
            SNew(SButton)
            .Text(LOCTEXT("ClearStamp", "Clear Stamp"))
            .ToolTipText(LOCTEXT("ClearStampTooltip", "Clear the generated preview mesh."))
            .OnClicked(this, &FGeometryStampActorDetails::ClearStamp)
        ]
    ];
}

FReply FGeometryStampActorDetails::ApplyPreset()
{
    for (const TWeakObjectPtr<AGeometryStampActor>& Actor : SelectedActors)
    {
        if (Actor.IsValid())
        {
            Actor->ApplyPreset();
        }
    }
    return FReply::Handled();
}

FReply FGeometryStampActorDetails::SaveCurrentAsPreset()
{
    if (SelectedActors.Num() == 1 && SelectedActors[0].IsValid())
    {
        SelectedActors[0]->SaveCurrentAsPreset();
    }
    return FReply::Handled();
}

FReply FGeometryStampActorDetails::ResetToDefaults()
{
    for (const TWeakObjectPtr<AGeometryStampActor>& Actor : SelectedActors)
    {
        if (Actor.IsValid())
        {
            Actor->ResetToDefaults();
        }
    }
    return FReply::Handled();
}

FReply FGeometryStampActorDetails::AutoConfigureFromMaterial()
{
    for (const TWeakObjectPtr<AGeometryStampActor>& Actor : SelectedActors)
    {
        if (Actor.IsValid())
        {
            Actor->AutoConfigureFromMaterial();
        }
    }
    return FReply::Handled();
}

FReply FGeometryStampActorDetails::RebuildStamp()
{
    for (const TWeakObjectPtr<AGeometryStampActor>& Actor : SelectedActors)
    {
        if (Actor.IsValid())
        {
            Actor->RebuildStamp();
        }
    }
    return FReply::Handled();
}

FReply FGeometryStampActorDetails::ClearStamp()
{
    for (const TWeakObjectPtr<AGeometryStampActor>& Actor : SelectedActors)
    {
        if (Actor.IsValid())
        {
            Actor->ClearStamp();
        }
    }
    return FReply::Handled();
}

bool FGeometryStampActorDetails::CanApplyPreset() const
{
    return SelectedActors.ContainsByPredicate([](const TWeakObjectPtr<AGeometryStampActor>& Actor)
    {
        return Actor.IsValid() && Actor->HasPreset();
    });
}

bool FGeometryStampActorDetails::CanSavePreset() const
{
    return SelectedActors.Num() == 1 && SelectedActors[0].IsValid();
}

#undef LOCTEXT_NAMESPACE
