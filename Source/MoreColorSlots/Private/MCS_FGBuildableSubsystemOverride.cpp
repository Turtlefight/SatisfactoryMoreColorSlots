
#include "MCS_FGBuildableSubsystemOverride.h"
#include "MCS_FGColoredInstanceManagerOverride.h"
#include "MoreColorSlotsModule.h"
#include "MCS_MoreColorSlotsConfigStruct.h"
#include "MCS_BPLibrary.h"

#include "SML/Public/Patching/NativeHookManager.h"
#include "FGBuildableSubsystem.h"
#include "FGColoredInstanceManager.h"
#include "FGColoredInstanceMeshProxy.h"
#include "FGGameState.h"
#include "FGSaveSession.h"

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

		FMCS_MoreColorSlotsConfigStruct config = FMCS_MoreColorSlotsConfigStruct::GetActiveConfig();
		EMCS_AdditionalColorPalettes additionalPalettes = (EMCS_AdditionalColorPalettes)config.NumColorPalettes;
		uint8 numColorSlots = UMCS_BPLibrary::GetNumberOfColorSlots(additionalPalettes);

		if (index < numColorSlots) {
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

		FMCS_MoreColorSlotsConfigStruct config = FMCS_MoreColorSlotsConfigStruct::GetActiveConfig();
		EMCS_AdditionalColorPalettes additionalPalettes = (EMCS_AdditionalColorPalettes)config.NumColorPalettes;
		uint8 numColorSlots = UMCS_BPLibrary::GetNumberOfColorSlots(additionalPalettes);

		if (index < numColorSlots) {
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

		FMCS_MoreColorSlotsConfigStruct config = FMCS_MoreColorSlotsConfigStruct::GetActiveConfig();
		EMCS_AdditionalColorPalettes additionalPalettes = (EMCS_AdditionalColorPalettes)config.NumColorPalettes;
		uint8 numColorSlots = UMCS_BPLibrary::GetNumberOfColorSlots(additionalPalettes);
		
		if (self->mColorSlotsPrimary_Linear.Num() > numColorSlots || self->mColorSlotsSecondary_Linear.Num() > numColorSlots) {
			UFGSaveSession* saveSession = UFGSaveSession::Get(self->GetWorld());
			check(saveSession);

			for (UObject* obj : saveSession->mLoadedObjects) {
				if (obj->GetClass()->ImplementsInterface(UFGColorInterface::StaticClass())) {
					if (IFGColorInterface::Execute_GetColorSlot(obj) >= numColorSlots) {
						IFGColorInterface::Execute_SetColorSlot(obj, 0);
					}
				}
			}

			self->mColorSlotsPrimary_Linear.SetNum(numColorSlots, true);
			self->mColorSlotsSecondary_Linear.SetNum(numColorSlots, true);
		}

		if (self->mColorSlotsPrimary_Linear.Num() < numColorSlots) {
			FLinearColor primaryColor(FColor(255, 149, 73, 255));
			for (int i = self->mColorSlotsPrimary_Linear.Num(); i < numColorSlots; i++) {
				self->mColorSlotsPrimary_Linear.Add(primaryColor);
			}
		}

		if (self->mColorSlotsSecondary_Linear.Num() < numColorSlots) {
			FLinearColor secondaryColor(FColor(95, 102, 140, 255));
			for (int i = self->mColorSlotsSecondary_Linear.Num(); i < numColorSlots; i++) {
				self->mColorSlotsSecondary_Linear.Add(secondaryColor);
			}
		}

		self->mNbPlayerExposedSlots = numColorSlots;
		
		AFGGameState* gameState = self->GetWorld()->GetGameState<AFGGameState>();
		if (gameState) {
			gameState->SetupColorSlots_Linear(self->mColorSlotsPrimary_Linear, self->mColorSlotsSecondary_Linear);
			if (gameState->mBuildingColorSlotsPrimary_Linear.Num() != numColorSlots || gameState->mBuildingColorSlotsSecondary_Linear.Num() != numColorSlots) {
				gameState->mHasInitializedColorSlots = false;
				gameState->mBuildingColorSlotsPrimary_Linear.Empty();
				gameState->mBuildingColorSlotsSecondary_Linear.Empty();
				gameState->SetupColorSlots_Linear(self->mColorSlotsPrimary_Linear, self->mColorSlotsSecondary_Linear);
			}
		}
		
	});
	/*
	SUBSCRIBE_METHOD_VIRTUAL(AFGBuildableSubsystem::PostLoadGame_Implementation, Cast<IFGSaveInterface>(bsCDO), [](auto& scope, AFGBuildableSubsystem* self, int32 saveVersion, int32 gameVersion) {
		scope(self, saveVersion, gameVersion);
	});
	*/
}
