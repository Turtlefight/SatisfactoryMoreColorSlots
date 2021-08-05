#pragma once
#include "CoreMinimal.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "MCS_MoreColorSlotsConfigStruct.generated.h"

/* Struct generated from Mod Configuration Asset '/MoreColorSlots/MCS_MoreColorSlotsConfig' */
USTRUCT(BlueprintType)
struct FMCS_MoreColorSlotsConfigStruct {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    bool EnableBetterColorPicker;

    UPROPERTY(BlueprintReadWrite)
    int32 NumColorPalettes;

    /* Retrieves active configuration value and returns object of this struct containing it */
    static FMCS_MoreColorSlotsConfigStruct GetActiveConfig() {
        FMCS_MoreColorSlotsConfigStruct ConfigStruct{};
        FConfigId ConfigId{"MoreColorSlots", ""};
        UConfigManager* ConfigManager = GEngine->GetEngineSubsystem<UConfigManager>();
        ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FMCS_MoreColorSlotsConfigStruct::StaticStruct(), &ConfigStruct});
        return ConfigStruct;
    }
};

