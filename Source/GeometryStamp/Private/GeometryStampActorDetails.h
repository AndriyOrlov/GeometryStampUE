#pragma once

#include "IDetailCustomization.h"

class AGeometryStampActor;
class FReply;

class FGeometryStampActorDetails final : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    TArray<TWeakObjectPtr<AGeometryStampActor>> SelectedActors;

    FReply ApplyPreset();
    FReply SaveCurrentAsPreset();
    FReply ResetToDefaults();
    FReply AutoConfigureFromMaterial();
    FReply RebuildStamp();
    FReply ClearStamp();
    bool CanApplyPreset() const;
    bool CanSavePreset() const;
};
