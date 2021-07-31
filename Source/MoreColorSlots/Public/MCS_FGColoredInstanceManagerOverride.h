#pragma once

#include "MoreColorSlotsModule.h"
#include "FGColoredInstanceManager.h"
#include "MCS_FGColoredInstanceManagerOverride.generated.h"

UCLASS()
class UMCS_FGColoredInstanceManagerOverride : public UFGColoredInstanceManager {
	GENERATED_BODY()

public:
	static void SetupHooks();


private:
	void OverrideSetupInstanceLists(UStaticMesh* staticMesh, bool makeSingleColor, bool useAsOccluder, EDistanceCullCategory CullCategory);
	void OverrideClearInstances();
	void OverrideAddInstance(const FTransform& transform, InstanceHandle& handle, uint8 colorIndex);
	void OverrideRemoveInstance(InstanceHandle& handle);
	void InitMaterialColorsForInstance(int additionalSlotIndex);

private:
	UPROPERTY()
	class UHierarchicalInstancedStaticMeshComponent* mAdditionalInstanceComponents[MCS_BUILDABLE_COLORS_MAX_SLOTS - BUILDABLE_COLORS_MAX_SLOTS];

	TArray<UFGColoredInstanceManager::InstanceHandle*> mAdditionalHandles[MCS_BUILDABLE_COLORS_MAX_SLOTS - BUILDABLE_COLORS_MAX_SLOTS];

	FVector2D mMinMaxCullDistanceCached;

	UPROPERTY()
	UStaticMesh* mStaticMeshCached;

	bool mUseAsOccluderCached;
};
