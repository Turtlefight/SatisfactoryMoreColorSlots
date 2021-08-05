
#include "MCS_FGColoredInstanceManagerOverride.h"
#include "MCS_BPLibrary.h"

#include "SML/Public/Patching/NativeHookManager.h"
#include "HAL/IConsoleManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "FGOptimizationSettings.h"
#include "FGBuildableSubsystem.h"
#include "FGFactoryMaterialInstanceManager.h"


/*
	To add more color slots we need to hook the implementation of UFGColoredInstanceManager,
	since it uses the BUILDABLE_COLORS_MAX_SLOTS macro as a constant for an array (which we can't easily make bigger).
	So to fix this problem, we add new arrays for the additional color slots
*/
void UMCS_FGColoredInstanceManagerOverride::SetupHooks() {
	SUBSCRIBE_METHOD(UFGColoredInstanceManager::SetupInstanceLists, [](auto& scope, UFGColoredInstanceManager* self, UStaticMesh* staticMesh, bool makeSingleColor, bool useAsOccluder, EDistanceCullCategory CullCategory) {
		scope(self, staticMesh, makeSingleColor, useAsOccluder, CullCategory);

		UMCS_FGColoredInstanceManagerOverride* subclass = CastChecked<UMCS_FGColoredInstanceManagerOverride>(self);
		subclass->OverrideSetupInstanceLists(staticMesh, makeSingleColor, useAsOccluder, CullCategory);
	});

	SUBSCRIBE_METHOD(UFGColoredInstanceManager::ClearInstances, [](auto& scope, UFGColoredInstanceManager* self) {
		scope(self);

		UMCS_FGColoredInstanceManagerOverride* subclass = CastChecked<UMCS_FGColoredInstanceManagerOverride>(self);
		subclass->ClearInstances();
	});

	SUBSCRIBE_METHOD(UFGColoredInstanceManager::AddInstance, [](auto& scope, UFGColoredInstanceManager* self, const FTransform& transform, InstanceHandle& handle, uint8 colorIndex) {
		if (colorIndex >= BUILDABLE_COLORS_MAX_SLOTS) {
			scope.Cancel();

			UMCS_FGColoredInstanceManagerOverride* subclass = CastChecked<UMCS_FGColoredInstanceManagerOverride>(self);
			subclass->OverrideAddInstance(transform, handle, colorIndex);
		}
	});

	SUBSCRIBE_METHOD(UFGColoredInstanceManager::RemoveInstance, [](auto& scope, UFGColoredInstanceManager* self, InstanceHandle& handle) {
		if (handle.ColorIndex >= BUILDABLE_COLORS_MAX_SLOTS) {
			scope.Cancel();

			UMCS_FGColoredInstanceManagerOverride* subclass = CastChecked<UMCS_FGColoredInstanceManagerOverride>(self);
			subclass->OverrideRemoveInstance(handle);
		}
	});

	// MoveInstance() only calls RemoveInstance + AddInstance, so should work out of the box.

	// UpdateMaterialColors() will be called from SetupInstanceLists,
	// we call the overriden one in OverrideSetupInstanceLists()

	// CreateHierarchicalInstancingComponent() only creates the components and doesn't care about the color slot.
}

void UMCS_FGColoredInstanceManagerOverride::OverrideSetupInstanceLists(UStaticMesh* staticMesh, bool makeSingleColor, bool useAsOccluder, EDistanceCullCategory CullCategory) {
	IConsoleManager& manager = IConsoleManager::Get();
	
	IConsoleVariable* renderFactorConVar = manager.FindConsoleVariable(TEXT("r.FactoryRenderFactor"));
	checkf(renderFactorConVar, TEXT("UMCS_FGColoredInstanceManagerOverride::OverrideSetupInstanceLists: IConsoleManager::FindConsoleVariable() failed"));

	UFGOptimizationSettings* optimizationSettings = UFGOptimizationSettings::GetPrivateStaticClass()->GetDefaultObject<UFGOptimizationSettings>();

	TConsoleVariableData<float>* renderFactorConVarData = renderFactorConVar->AsVariableFloat();
	float renderFactor = renderFactorConVarData->GetValueOnGameThread();

	FVector2D minMaxCullDistance = optimizationSettings->GetCullDistanceByCategory(this->mCullCategory, renderFactor);

	// Don't create instances directly - only create them when they first become necessary
	/*
	if (!this->mSingleColorOnly) {
		for (int i = 0; i < MCS_BUILDABLE_COLORS_MAX_SLOTS - BUILDABLE_COLORS_MAX_SLOTS; i++) {
			this->mAdditionalInstanceComponents[i] = CreateHierarchicalInstancingComponent(staticMesh, useAsOccluder, minMaxCullDistance);
		}
	}
	*/
	this->mStaticMeshCached = staticMesh;
	this->mUseAsOccluderCached = useAsOccluder;
	this->mMinMaxCullDistanceCached = minMaxCullDistance;

	FMCS_MoreColorSlotsConfigStruct config = FMCS_MoreColorSlotsConfigStruct::GetActiveConfig();
	EMCS_AdditionalColorPalettes additionalPalettes = (EMCS_AdditionalColorPalettes)config.NumColorPalettes;
	uint8 numColorSlots = UMCS_BPLibrary::GetNumberOfColorSlots(additionalPalettes);
	this->mNumColorSlotsCached = numColorSlots;
}

void UMCS_FGColoredInstanceManagerOverride::OverrideClearInstances() {
	for (int i = 0; i < MCS_BUILDABLE_COLORS_MAX_SLOTS - BUILDABLE_COLORS_MAX_SLOTS; i++) {
		if (this->mAdditionalInstanceComponents[i])
			this->mAdditionalInstanceComponents[i]->ClearInstances();
	}
}

void UMCS_FGColoredInstanceManagerOverride::OverrideAddInstance(const FTransform& transform, InstanceHandle& handle, uint8 colorIndex) {
	checkf(!this->mSingleColorOnly, TEXT("UFGColoredInstanceManager::OverrideAddInstance: called override on single color instance"));
	checkf(colorIndex >= BUILDABLE_COLORS_MAX_SLOTS && colorIndex < MCS_BUILDABLE_COLORS_MAX_SLOTS, TEXT("UFGColoredInstanceManager::OverrideAddInstance: colorIndex out of range"));

	int additionalColorIndex = colorIndex - BUILDABLE_COLORS_MAX_SLOTS;

	// this only happens when decreasing the number of color slots in the mod config.
	// we just forward to the vanilla color slot until the instance gets assigned its new color slot
	if (additionalColorIndex >= this->mNumColorSlotsCached) {
		AddInstance(transform, handle, 0);
		return;
	}

	if (!this->mSingleColorOnly && !this->mAdditionalInstanceComponents[additionalColorIndex]) {
		this->mAdditionalInstanceComponents[additionalColorIndex] = CreateHierarchicalInstancingComponent(this->mStaticMeshCached, this->mUseAsOccluderCached, this->mMinMaxCullDistanceCached);
		InitMaterialColorsForInstance(additionalColorIndex);
	}

	if (this->mAdditionalInstanceComponents[additionalColorIndex]) {
		checkf(!handle.IsInstanced(), TEXT("UMCS_FGColoredInstanceManagerOverride::OverrideAddInstance: Handle is already instanced!"));

		int index = this->mAdditionalInstanceComponents[additionalColorIndex]->AddInstance(transform);
		handle.HandleID = index;

		int arrIdx = this->mAdditionalHandles[additionalColorIndex].Add(&handle);
		checkf(arrIdx == index, TEXT("UMCS_FGColoredInstanceManagerOverride::OverrideAddInstance: HandleId mismatch"));
		handle.ColorIndex = colorIndex;
		this->mAdditionalInstanceComponents[additionalColorIndex]->MarkRenderStateDirty();
	}
}

void UMCS_FGColoredInstanceManagerOverride::OverrideRemoveInstance(InstanceHandle& handle) {
	checkf(handle.IsInstanced(), TEXT("UMCS_FGColoredInstanceManagerOverride::OverrideRemoveInstance: handle ist not instanced!"));

	int additionalColorIndex = handle.ColorIndex - BUILDABLE_COLORS_MAX_SLOTS;
	checkf(handle.HandleID < this->mAdditionalHandles[additionalColorIndex].Num(), TEXT("UMCS_FGColoredInstanceManagerOverride::OverrideRemoveInstance: handle out of range!"));
	checkf(this->mAdditionalHandles[additionalColorIndex][handle.HandleID] == &handle, TEXT("UMCS_FGColoredInstanceManagerOverride::OverrideRemoveInstance: mAdditionalHandles out of sync!"));

	this->mAdditionalHandles[additionalColorIndex].RemoveAtSwap(handle.HandleID, 1, true);

	int numElements = this->mAdditionalHandles[additionalColorIndex].Num();
	if (handle.HandleID < numElements) {
		checkf(this->mAdditionalHandles[additionalColorIndex][handle.HandleID]->IsInstanced(), TEXT("UMCS_FGColoredInstanceManagerOverride::OverrideRemoveInstance: swap element is not instanced!"));
		this->mAdditionalHandles[additionalColorIndex][handle.HandleID]->HandleID = handle.HandleID;
	}

	this->mAdditionalInstanceComponents[additionalColorIndex]->RemoveInstance(handle.HandleID);
	if (this->mAdditionalHandles[additionalColorIndex].Num() < 1) {
		this->mAdditionalInstanceComponents[additionalColorIndex]->ClearInstances();
		this->mAdditionalInstanceComponents[additionalColorIndex]->BuildTreeIfOutdated(false, true);
	}
	else {
		this->mAdditionalInstanceComponents[additionalColorIndex]->MarkRenderStateDirty();
	}

	handle.HandleID = INDEX_NONE;
}

void UMCS_FGColoredInstanceManagerOverride::InitMaterialColorsForInstance(int additionalSlotIndex) {
	if (!this->mSingleColorOnly) {
		UWorld* world = GetWorld();
		AFGBuildableSubsystem* buildableSubsystem = AFGBuildableSubsystem::Get(world);
		if (buildableSubsystem) {
			UHierarchicalInstancedStaticMeshComponent* hismc = this->mAdditionalInstanceComponents[additionalSlotIndex];
			int numMaterials = hismc->GetMaterials().Num();
			for (int i = 0; i < numMaterials; i++) {
				UMaterialInterface* material = hismc->GetMaterial(i);

				FLinearColor dummyColor;
				bool found = material->GetVectorParameterValue(
					FHashedMaterialParameterInfo(FHashedName(PrimaryColor), GlobalParameter, -1),
					dummyColor,
					false
				);

				if (!found) {
					found = material->GetVectorParameterValue(
						FHashedMaterialParameterInfo(FHashedName(SecondaryColor), GlobalParameter, -1),
						dummyColor,
						false
					);
				}

				if (found) {
					FString materialName;
					material->GetFName().ToString(materialName);
					FString empty;

					UFGFactoryMaterialInstanceManager* miManager = buildableSubsystem->GetOrCreateMaterialManagerForMaterialInterface(
						material,
						materialName,
						empty,
						true,
						nullptr,
						nullptr
					);

					if (!miManager) {
						FLinearColor linearPrimaryColor = buildableSubsystem->GetColorSlotPrimary_Linear(BUILDABLE_COLORS_MAX_SLOTS + additionalSlotIndex);
						hismc->SetVectorParameterValueOnMaterials(PrimaryColor, FVector(linearPrimaryColor));
						FLinearColor linearSecondaryColor = buildableSubsystem->GetColorSlotSecondary_Linear(BUILDABLE_COLORS_MAX_SLOTS + additionalSlotIndex);
						hismc->SetVectorParameterValueOnMaterials(SecondaryColor, FVector(linearSecondaryColor));
					}
					else {
						UMaterialInstanceDynamic* newMaterialInstance = miManager->GetMaterialForIndex(BUILDABLE_COLORS_MAX_SLOTS + additionalSlotIndex);
						if (newMaterialInstance) {
							hismc->SetMaterial(i, newMaterialInstance);
						}
					}
				}
			}
		}
	}
}
