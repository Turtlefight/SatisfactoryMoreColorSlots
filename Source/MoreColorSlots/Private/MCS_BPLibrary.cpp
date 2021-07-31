
#include "MCS_BPLibrary.h"

#include "FGColorInterface.h"
#include "FGBuildableSubsystem.h"
#include "Kismet/GameplayStatics.h"


void UMCS_BPLibrary::ResetColorSlotsToVanilla(UObject* WorldContextObject) {
	TArray<AActor*> coloredStuff;
	UGameplayStatics::GetAllActorsWithInterface(WorldContextObject, UFGColorInterface::StaticClass(), coloredStuff);

	for (AActor* actor : coloredStuff) {
		if (IFGColorInterface::Execute_GetColorSlot(actor) >= BUILDABLE_COLORS_MAX_SLOTS) {
			IFGColorInterface::Execute_SetColorSlot(actor, 0);
		}
	}

	AFGBuildableSubsystem* buildSystem = AFGBuildableSubsystem::Get(WorldContextObject);
	if (buildSystem) {
		buildSystem->mColorSlotsPrimary_Linear.SetNum(BUILDABLE_COLORS_MAX_SLOTS, true);
		buildSystem->mColorSlotsSecondary_Linear.SetNum(BUILDABLE_COLORS_MAX_SLOTS, true);
		buildSystem->mNbPlayerExposedSlots = 16;
	}
}