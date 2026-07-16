#pragma once

#include "Modules/ModuleManager.h"

class FSlateStyleSet;

class FGeometryStampModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterStyle();
    void UnregisterStyle();
    void RegisterDetailsCustomization();
    void UnregisterDetailsCustomization();
    void RegisterPlacementItems();
    void UnregisterPlacementItems();

    TSharedPtr<FSlateStyleSet> Style;
};
