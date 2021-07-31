
#include "MCS_FGBuildableSubsystemOverride.h"
#include "MCS_FGColoredInstanceManagerOverride.h"
#include "MoreColorSlotsModule.h"

#include "SML/Public/Patching/NativeHookManager.h"
#include "FGBuildableSubsystem.h"
#include "FGColoredInstanceManager.h"
#include "FGColoredInstanceMeshProxy.h"
#include "FGGameState.h"

void MCS_SetupFGBuildableSubsystemOverrides() {
	/*
		Hook into the Colored Instance Manager Creation,
		so we can substitute the default FGColoredInstanceManager class with our extended one.
	*/
	SUBSCRIBE_METHOD(AFGBuildableSubsystem::GetColoredInstanceManager, [](auto& scope, AFGBuildableSubsystem* self, UFGColoredInstanceMeshProxy* proxy) {
		
		if (proxy && proxy->GetStaticMesh()) {
			UStaticMesh* mesh = proxy->GetStaticMesh();
			
			UFGColoredInstanceManager** manager = self->mColoredInstances.Find(mesh);
			if (manager) {
				checkf(*manager, TEXT("SetupFGBuildableSubsystemOverrides: AFGBuildableSubsystem::GetColoredInstanceManager: mColoredInstances contained nullptr!"));
				scope.Override(*manager);
				return;
			}
			
			if (!self->mBuildableInstancesActor) {
				self->mBuildableInstancesActor = self->GetWorld()->SpawnActor<AActor>();
			}

			UMCS_FGColoredInstanceManagerOverride* coloredInstanceManager = NewObject<UMCS_FGColoredInstanceManagerOverride>(
				self->mBuildableInstancesActor,
				UMCS_FGColoredInstanceManagerOverride::StaticClass(),
				NAME_None,
				RF_NoFlags,
				nullptr,
				false,
				nullptr
			);
			coloredInstanceManager->SetupAttachment(self->mBuildableInstancesActor->GetRootComponent());
			coloredInstanceManager->SetMobility(EComponentMobility::Static);
			coloredInstanceManager->RegisterComponent();

			coloredInstanceManager->SetupInstanceLists(mesh, !proxy->mCanBecolored, proxy->bUseAsOccluder, proxy->mOptimizationCategory);

			self->mColoredInstances.Emplace(mesh, coloredInstanceManager);
			scope.Override(coloredInstanceManager);
			return;
			
		}

		scope.Override(nullptr);
	});

	// Getters only check the array size, but the setters also have an explicit check for BUILDABLE_COLORS_MAX_SLOTS
	// so we need to replace both setters

	SUBSCRIBE_METHOD(AFGBuildableSubsystem::SetColorSlotPrimary_Linear, [](auto& scope, AFGBuildableSubsystem* self, uint8 index, FLinearColor color) {
		scope.Cancel();

		if (index < MCS_BUILDABLE_COLORS_MAX_SLOTS) {
			if (index >= self->mColorSlotsPrimary_Linear.Num()) {
				UE_LOG(LogMoreColorSlots, Error, TEXT("Failed To Set primary Slot! Invalid index: [%d]"), index);
				return;
			}

			self->mColorSlotsAreDirty = true;
			self->mColorSlotsPrimary_Linear[index] = color;
		}
	});


	SUBSCRIBE_METHOD(AFGBuildableSubsystem::SetColorSlotSecondary_Linear, [](auto& scope, AFGBuildableSubsystem* self, uint8 index, FLinearColor color) {
		scope.Cancel();

		if (index < MCS_BUILDABLE_COLORS_MAX_SLOTS) {
			if (index >= self->mColorSlotsSecondary_Linear.Num()) {
				UE_LOG(LogMoreColorSlots, Error, TEXT("Failed To Set secondary Slot! Invalid index: [%d]"), index);
				return;
			}

			self->mColorSlotsAreDirty = true;
			self->mColorSlotsSecondary_Linear[index] = color;
		}
	});

	// Setup additional color slots
	AFGBuildableSubsystem* bsCDO = GetMutableDefault<AFGBuildableSubsystem>();
	SUBSCRIBE_METHOD_VIRTUAL(AFGBuildableSubsystem::BeginPlay, bsCDO, [](auto& scope, AFGBuildableSubsystem* self) {
		scope(self);

		if (self->GetLocalRole() != ROLE_Authority)
			return;
		
		if (self->mColorSlotsPrimary_Linear.Num() < MCS_BUILDABLE_COLORS_MAX_SLOTS) {
			FLinearColor primaryColor(FColor(255, 149, 73, 255));
			for (int i = self->mColorSlotsPrimary_Linear.Num(); i < MCS_BUILDABLE_COLORS_MAX_SLOTS; i++) {
				self->mColorSlotsPrimary_Linear.Add(primaryColor);
			}
		}

		if (self->mColorSlotsSecondary_Linear.Num() < MCS_BUILDABLE_COLORS_MAX_SLOTS) {
			FLinearColor secondaryColor(FColor(95, 102, 140, 255));
			for (int i = self->mColorSlotsSecondary_Linear.Num(); i < MCS_BUILDABLE_COLORS_MAX_SLOTS; i++) {
				self->mColorSlotsSecondary_Linear.Add(secondaryColor);
			}
		}

		self->mNbPlayerExposedSlots = MCS_BUILDABLE_COLORS_MAX_SLOTS;
		
		AFGGameState* gameState = self->GetWorld()->GetGameState<AFGGameState>();
		if (gameState) {
			gameState->SetupColorSlots_Linear(self->mColorSlotsPrimary_Linear, self->mColorSlotsSecondary_Linear);
			if (gameState->mBuildingColorSlotsPrimary_Linear.Num() != MCS_BUILDABLE_COLORS_MAX_SLOTS || gameState->mBuildingColorSlotsSecondary_Linear.Num() != MCS_BUILDABLE_COLORS_MAX_SLOTS) {
				gameState->mHasInitializedColorSlots = false;
				gameState->mBuildingColorSlotsPrimary_Linear.Empty();
				gameState->mBuildingColorSlotsSecondary_Linear.Empty();
				gameState->SetupColorSlots_Linear(self->mColorSlotsPrimary_Linear, self->mColorSlotsSecondary_Linear);
			}
		}
		
	});
}
