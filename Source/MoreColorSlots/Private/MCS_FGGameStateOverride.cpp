
#include "MCS_FGGameStateOverride.h"
#include "MoreColorSlotsModule.h"
#include "MCS_MoreColorSlotsConfigStruct.h"

#include "SML/Public/Patching/NativeHookManager.h"
#include "FGGameState.h"

void MCS_SetupFGGameStateOverrides() {
	SUBSCRIBE_METHOD(AFGGameState::OnRep_BuildingColorSlotPrimary_Linear, [](auto& scope, AFGGameState* self) {
		scope.Cancel();

		AFGBuildableSubsystem* buildSubsystem = AFGBuildableSubsystem::Get(self->GetWorld());

		FMCS_MoreColorSlotsConfigStruct config = FMCS_MoreColorSlotsConfigStruct::GetActiveConfig();
		EMCS_AdditionalColorPalettes additionalPalettes = (EMCS_AdditionalColorPalettes)config.NumColorPalettes;
		uint8 numColorSlots = UMCS_BPLibrary::GetNumberOfColorSlots(additionalPalettes);

		for (int i = 0; i < numColorSlots; i++) {
			check(i < self->mBuildingColorSlotsPrimary_Linear.Num());

			FLinearColor buildColor = buildSubsystem->GetColorSlotPrimary_Linear(i);
			FLinearColor stateColor = self->mBuildingColorSlotsPrimary_Linear[i];

			if (buildColor != stateColor) {
				buildSubsystem->SetColorSlotPrimary_Linear(i, stateColor);
			}
		}
	});

	SUBSCRIBE_METHOD(AFGGameState::OnRep_BuildingColorSlotSecondary_Linear, [](auto& scope, AFGGameState* self) {
		scope.Cancel();

		AFGBuildableSubsystem* buildSubsystem = AFGBuildableSubsystem::Get(self->GetWorld());

		FMCS_MoreColorSlotsConfigStruct config = FMCS_MoreColorSlotsConfigStruct::GetActiveConfig();
		EMCS_AdditionalColorPalettes additionalPalettes = (EMCS_AdditionalColorPalettes)config.NumColorPalettes;
		uint8 numColorSlots = UMCS_BPLibrary::GetNumberOfColorSlots(additionalPalettes);

		for (int i = 0; i < numColorSlots; i++) {
			check(i < self->mBuildingColorSlotsSecondary_Linear.Num());

			FLinearColor buildColor = buildSubsystem->GetColorSlotSecondary_Linear(i);
			FLinearColor stateColor = self->mBuildingColorSlotsSecondary_Linear[i];

			if (buildColor != stateColor) {
				buildSubsystem->SetColorSlotSecondary_Linear(i, stateColor);
			}
		}
	});
}
