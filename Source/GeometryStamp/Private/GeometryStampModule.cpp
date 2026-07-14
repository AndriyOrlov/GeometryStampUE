#include "GeometryStampModule.h"

#include "GeometryStampActor.h"
#include "GeometryStampActorDetails.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FGeometryStampModule"

void FGeometryStampModule::StartupModule()
{
    RegisterDetailsCustomization();
}

void FGeometryStampModule::ShutdownModule()
{
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGeometryStampModule, GeometryStamp)
