
#include "MoreColorSlotsModule.h"
#include "MCS_FGColoredInstanceManagerOverride.h"
#include "MCS_FGBuildableSubsystemOverride.h"
#include "MCS_ColorGunWidgetOverride.h"
#include "MCS_FGGameStateOverride.h"
#include "MCS_FGFactoryMaterialInstanceManagerOverride.h"


#include "SML/Public/Patching/NativeHookManager.h"
#include "FGGameMode.h"

DEFINE_LOG_CATEGORY(LogMoreColorSlots);

void FMoreColorSlotsModule::StartupModule() {
#if !WITH_EDITOR
	UMCS_FGColoredInstanceManagerOverride::SetupHooks();
	MCS_SetupFGBuildableSubsystemOverrides();
	MCS_SetupFGGameStateOverrides();
	MCS_SetupFGFactoryMaterialInstanceManagerOverrides();

	// delay color gun widget override
	AFGGameMode* gameModeCDO = GetMutableDefault<AFGGameMode>();
	SUBSCRIBE_METHOD_VIRTUAL(AFGGameMode::PostLogin, gameModeCDO, [](auto& scope, AFGGameMode* self, APlayerController* playerController) {
		MCS_SetupColorGunWidgetOverride();
	});
#endif
}

IMPLEMENT_GAME_MODULE(FMoreColorSlotsModule, MoreColorSlots);
