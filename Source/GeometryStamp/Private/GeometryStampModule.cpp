#include "GeometryStampModule.h"

#include "GeometryStampActor.h"
#include "GeometryStampActorDetails.h"
#include "ActorFactories/ActorFactoryBlueprint.h"
#include "ActorFactories/ActorFactoryClass.h"
#include "AssetRegistry/AssetData.h"
#include "Brushes/SlateImageBrush.h"
#include "Engine/Blueprint.h"
#include "IPlacementModeModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

#define LOCTEXT_NAMESPACE "FGeometryStampModule"

namespace
{
const FName GeometryStampPlacementCategory(TEXT("GeometryStamp"));
}

void FGeometryStampModule::StartupModule()
{
    if (IsRunningCommandlet() || !FApp::CanEverRender())
    {
        return;
    }

    RegisterStyle();
    RegisterDetailsCustomization();
    RegisterPlacementItems();
}

void FGeometryStampModule::ShutdownModule()
{
    if (IsRunningCommandlet() || !FApp::CanEverRender())
    {
        return;
    }

    UnregisterPlacementItems();
    UnregisterDetailsCustomization();
    UnregisterStyle();
}

void FGeometryStampModule::RegisterStyle()
{
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("GeometryStamp"));
    if (!Plugin)
    {
        return;
    }

    Style = MakeShared<FSlateStyleSet>(TEXT("GeometryStampStyle"));
    Style->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));
    const FString IconPath = Style->RootToContentDir(TEXT("Icon128"), TEXT(".png"));
    Style->Set(TEXT("ClassIcon.GeometryStampActor"), new FSlateImageBrush(IconPath, FVector2D(16.0f)));
    Style->Set(TEXT("ClassThumbnail.GeometryStampActor"), new FSlateImageBrush(IconPath, FVector2D(64.0f)));
    FSlateStyleRegistry::RegisterSlateStyle(*Style);
}

void FGeometryStampModule::UnregisterStyle()
{
    if (Style)
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*Style);
        Style.Reset();
    }
}

void FGeometryStampModule::RegisterDetailsCustomization()
{
    FPropertyEditorModule& PropertyEditorModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

    PropertyEditorModule.RegisterCustomClassLayout(
        AGeometryStampActor::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FGeometryStampActorDetails::MakeInstance));

    PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FGeometryStampModule::UnregisterDetailsCustomization()
{
    if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
    {
        return;
    }

    FPropertyEditorModule& PropertyEditorModule =
        FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
    PropertyEditorModule.UnregisterCustomClassLayout(AGeometryStampActor::StaticClass()->GetFName());
}

void FGeometryStampModule::RegisterPlacementItems()
{
    IPlacementModeModule& PlacementModeModule = IPlacementModeModule::Get();

    const FPlacementCategoryInfo CategoryInfo(
        LOCTEXT("GeometryStampPlacementCategory", "Geometry Stamp"),
        GeometryStampPlacementCategory,
        TEXT("PMGeometryStamp"),
        25,
        true);

    PlacementModeModule.RegisterPlacementCategory(CategoryInfo);

    const TSharedRef<FPlaceableItem> GeometryStampItem = MakeShared<FPlaceableItem>(
        *UActorFactoryClass::StaticClass(),
        FAssetData(AGeometryStampActor::StaticClass()),
        TEXT("ClassThumbnail.GeometryStampActor"),
        TEXT("ClassIcon.GeometryStampActor"),
        TOptional<FLinearColor>(FLinearColor(1.0f, 0.24f, 0.04f)),
        TOptional<int32>(0),
        TOptional<FText>(LOCTEXT("GeometryStampPlaceableName", "Geometry Stamp")));

    PlacementModeModule.RegisterPlaceableItem(GeometryStampPlacementCategory, GeometryStampItem);

    struct FExamplePlacement
    {
        const TCHAR* AssetPath;
        const TCHAR* Label;
    };
    static const FExamplePlacement Examples[] = {
        {TEXT("/GeometryStamp/Examples/Blueprints/BP_GS_Crater.BP_GS_Crater"), TEXT("Example — Crater")},
        {TEXT("/GeometryStamp/Examples/Blueprints/BP_GS_RockShelf.BP_GS_RockShelf"), TEXT("Example — Rock Shelf")},
        {TEXT("/GeometryStamp/Examples/Blueprints/BP_GS_GroundPatch.BP_GS_GroundPatch"), TEXT("Example — Ground Patch")},
    };

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Examples); ++Index)
    {
        if (UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, Examples[Index].AssetPath))
        {
            const TSharedRef<FPlaceableItem> ExampleItem = MakeShared<FPlaceableItem>(
                *UActorFactoryBlueprint::StaticClass(),
                FAssetData(Blueprint),
                TEXT("ClassThumbnail.GeometryStampActor"),
                TEXT("ClassIcon.GeometryStampActor"),
                TOptional<FLinearColor>(FLinearColor(1.0f, 0.24f, 0.04f)),
                TOptional<int32>(Index + 1),
                TOptional<FText>(FText::FromString(Examples[Index].Label)));
            PlacementModeModule.RegisterPlaceableItem(GeometryStampPlacementCategory, ExampleItem);
        }
    }
}

void FGeometryStampModule::UnregisterPlacementItems()
{
    if (IPlacementModeModule::IsAvailable())
    {
        IPlacementModeModule::Get().UnregisterPlacementCategory(GeometryStampPlacementCategory);
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGeometryStampModule, GeometryStamp)
