#include "GeometryStampActorDetails.h"

#include "GeometryStampActor.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/MessageDialog.h"
#include "ObjectTools.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GeometryStampActorDetails"

namespace
{
class SGeometryStampBakeWindow final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGeometryStampBakeWindow) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AGeometryStampActor>, StampActor)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        StampActor = InArgs._StampActor;
        const FString DefaultName = StampActor.IsValid()
            ? TEXT("SM_") + ObjectTools::SanitizeObjectName(StampActor->GetActorLabel())
            : TEXT("SM_GeometryStamp");

        ChildSlot
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 12.0f, 12.0f, 4.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("BakeFolderLabel", "Content Folder"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 0.0f, 12.0f, 8.0f)
            [
                SAssignNew(FolderTextBox, SEditableTextBox)
                .Text(FText::FromString(TEXT("/Game/GeometryStamps")))
                .ToolTipText(LOCTEXT("BakeFolderTooltip", "Content Browser folder. It must start with /Game/."))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 4.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("BakeNameLabel", "Asset Name"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 0.0f, 12.0f, 8.0f)
            [
                SAssignNew(NameTextBox, SEditableTextBox)
                .Text(FText::FromString(DefaultName))
                .SelectAllTextWhenFocused(true)
                .ToolTipText(LOCTEXT("BakeNameTooltip", "A new Static Mesh asset name. Existing assets are never overwritten."))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 4.0f)
            [
                SAssignNew(NaniteCheckBox, SCheckBox)
                .IsChecked(ECheckBoxState::Checked)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("BakeNanite", "Enable Nanite"))
                    .ToolTipText(LOCTEXT("BakeNaniteTooltip", "Enable Nanite on the baked Static Mesh asset."))
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 4.0f)
            [
                SAssignNew(ReplaceActorCheckBox, SCheckBox)
                .IsChecked(ECheckBoxState::Checked)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("BakeReplaceActor", "Replace stamp actor in the level"))
                    .ToolTipText(LOCTEXT("BakeReplaceActorTooltip", "Place the baked Static Mesh Actor at the same transform and remove the preview actor. Undo restores the stamp actor."))
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(12.0f, 12.0f)
            .HAlign(HAlign_Right)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("BakeCancel", "Cancel"))
                    .OnClicked(this, &SGeometryStampBakeWindow::CloseWindow)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("BakeConfirm", "Bake Static Mesh"))
                    .IsEnabled_Lambda([this]() { return StampActor.IsValid(); })
                    .OnClicked(this, &SGeometryStampBakeWindow::Bake)
                ]
            ]
        ];
    }

private:
    FReply Bake()
    {
        AGeometryStampActor* Actor = StampActor.Get();
        if (!Actor)
        {
            return CloseWindow();
        }

        FText Error;
        const bool bSucceeded = Actor->BakeToStaticMesh(
            FolderTextBox->GetText().ToString(),
            NameTextBox->GetText().ToString(),
            NaniteCheckBox->IsChecked(),
            ReplaceActorCheckBox->IsChecked(),
            Error);
        if (!bSucceeded)
        {
            FMessageDialog::Open(EAppMsgType::Ok, Error, LOCTEXT("BakeErrorTitle", "Geometry Stamp Bake"));
            return FReply::Handled();
        }

        return CloseWindow();
    }

    FReply CloseWindow()
    {
        if (const TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(AsShared()))
        {
            Window->RequestDestroyWindow();
        }
        return FReply::Handled();
    }

    TWeakObjectPtr<AGeometryStampActor> StampActor;
    TSharedPtr<SEditableTextBox> FolderTextBox;
    TSharedPtr<SEditableTextBox> NameTextBox;
    TSharedPtr<SCheckBox> NaniteCheckBox;
    TSharedPtr<SCheckBox> ReplaceActorCheckBox;
};
}

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

    GeometryStampCategory.AddCustomRow(LOCTEXT("BakeFilter", "Bake Static Mesh"))
    .WholeRowContent()
    [
        SNew(SButton)
        .Text(LOCTEXT("OpenBakeWindow", "Bake to Static Mesh..."))
        .ToolTipText(LOCTEXT("OpenBakeWindowTooltip", "Create a new Static Mesh asset from the current projected preview. Existing assets are never overwritten."))
        .HAlign(HAlign_Center)
        .IsEnabled(this, &FGeometryStampActorDetails::CanBake)
        .OnClicked(this, &FGeometryStampActorDetails::OpenBakeWindow)
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

FReply FGeometryStampActorDetails::OpenBakeWindow()
{
    if (!CanBake())
    {
        return FReply::Handled();
    }

    const TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(LOCTEXT("BakeWindowTitle", "Bake Geometry Stamp"))
        .ClientSize(FVector2D(460.0f, 270.0f))
        .SupportsMinimize(false)
        .SupportsMaximize(false)
        [
            SNew(SGeometryStampBakeWindow)
            .StampActor(SelectedActors[0])
        ];
    FSlateApplication::Get().AddWindow(Window);
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

bool FGeometryStampActorDetails::CanBake() const
{
    return SelectedActors.Num() == 1
        && SelectedActors[0].IsValid()
        && SelectedActors[0]->HasBakeableMesh();
}

#undef LOCTEXT_NAMESPACE
