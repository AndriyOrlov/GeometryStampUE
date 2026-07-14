#include "GeometryStampModule.h"

#include "GeometryStampActor.h"
#include "GeometryStampActorDetails.h"
#include "ActorFactories/ActorFactoryClass.h"
#include "AssetRegistry/AssetData.h"
#include "IPlacementModeModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FGeometryStampModule"

namespace
{
const FName GeometryStampPlacementCategory(TEXT("GeometryStamp"));
}

void FGeometryStampModule::StartupModule()
{
    RegisterDetailsCustomization();
    RegisterPlacementItems();
}

void FGeometryStampModule::ShutdownModule()
{
    UnregisterPlacementItems();
    UnregisterDetailsCustomization();
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
        NAME_None,
        NAME_None,
        TOptional<FLinearColor>(FLinearColor(1.0f, 0.24f, 0.04f)),
        TOptional<int32>(0),
        TOptional<FText>(LOCTEXT("GeometryStampPlaceableName", "Geometry Stamp")));

    PlacementModeModule.RegisterPlaceableItem(GeometryStampPlacementCategory, GeometryStampItem);
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
