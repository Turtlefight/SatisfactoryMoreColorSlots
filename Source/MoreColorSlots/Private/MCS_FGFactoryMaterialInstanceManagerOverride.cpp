
#include "MCS_FGFactoryMaterialInstanceManagerOverride.h"
#include "MoreColorSlotsModule.h"
#include "MCS_BPLibrary.h"

#include "SML/Public/Patching/NativeHookManager.h"
#include "FGFactoryMaterialInstanceManager.h"
#include "FGBuildableSubsystem.h"
#include "Buildables/FGBuildable.h"

void MCS_SetupFGFactoryMaterialInstanceManagerOverrides() {
	SUBSCRIBE_METHOD(UFGFactoryMaterialInstanceManager::Init, [](
		auto& scope, UFGFactoryMaterialInstanceManager* self,
		UMaterialInterface* materialInterface,
		FString& lookupName,
		FString& lookupPrefix,
		UWorld* worldContext,
		bool canBeColored,
		AFGBuildable* forBuildable)
	{
		scope.Cancel();

		FMCS_MoreColorSlotsConfigStruct config = FMCS_MoreColorSlotsConfigStruct::GetActiveConfig();
		EMCS_AdditionalColorPalettes additionalPalettes = (EMCS_AdditionalColorPalettes)config.NumColorPalettes;
		uint8 numColorSlots = UMCS_BPLibrary::GetNumberOfColorSlots(additionalPalettes);

		self->mCanBeColored = canBeColored;
		self->mIsShared = lookupPrefix.IsEmpty();
		self->mMaterialInstances.SetNumZeroed(self->mCanBeColored ? numColorSlots : 1);
		
		self->mCachedBuildSystem = AFGBuildableSubsystem::Get(worldContext);
		if (self->mCachedBuildSystem) {
			if (!materialInterface) {
				FString buildableName = TEXT("nullptr");
				if (forBuildable)
					buildableName = forBuildable->GetFName().ToString();

				UE_LOG(LogMoreColorSlots, Warning, TEXT("Using default material for %s in %s! This should never happen! Figure it oot."), TEXT("materialInterface = nullpeter"), *buildableName);

				UMaterial* defaultMaterial = self->mCachedBuildSystem->mDefaultFactoryMaterial;
				// FString matName = FString::Printf(TEXT("FMIM_default_%s"), *lookupName);
				if (!self->mCanBeColored) {
					UMaterialInstanceDynamic* dynMat = UMaterialInstanceDynamic::Create(defaultMaterial, nullptr);
					self->mMaterialInstances[0] = dynMat;
					self->mInstanceNames.Add(dynMat->GetFName().ToString());
				} else {
					for (int i = 0; i < numColorSlots; i++) {
						UMaterialInstanceDynamic* dynMat = UMaterialInstanceDynamic::Create(defaultMaterial, nullptr);
						self->mMaterialInstances[i] = dynMat;
						self->mInstanceNames.Add(dynMat->GetFName().ToString());
					}
				}
			} else {
				self->mInitializingFromInterface = materialInterface;
				// FString matName = FString::Printf(TEXT("FMIM_%s"), *lookupName);
				if (!self->mCanBeColored) {
					UMaterialInstanceDynamic* dynMat;
					if (!materialInterface->IsA(UMaterialInstanceDynamic::StaticClass())) {
						dynMat = UMaterialInstanceDynamic::Create(materialInterface, nullptr);
					} else {
						UMaterial* baseMat = materialInterface->GetMaterial();
						dynMat = UMaterialInstanceDynamic::Create(baseMat, nullptr);
					}
					self->mMaterialInstances[0] = dynMat;

					FString dynMatName = dynMat->GetFName().ToString();
					if(!lookupPrefix.IsEmpty())
						dynMatName = FString::Printf(TEXT("%s_%s"), *lookupPrefix, *dynMatName);
					self->mInstanceNames.Add(dynMatName);
					
				} else {
					for (int i = 0; i < numColorSlots; i++) {
						UMaterialInstanceDynamic* dynMat;
						if (!materialInterface->IsA(UMaterialInstanceDynamic::StaticClass())) {
							dynMat = UMaterialInstanceDynamic::Create(materialInterface, nullptr);
						}
						else {
							UMaterial* baseMat = materialInterface->GetMaterial();
							dynMat = UMaterialInstanceDynamic::Create(baseMat, nullptr);
						}

						self->mMaterialInstances[i] = dynMat;
						FString dynMatName = dynMat->GetFName().ToString();
						
						if(!lookupPrefix.IsEmpty())
							dynMatName = FString::Printf(TEXT("%s_%s"), *lookupPrefix, *dynMatName);
						self->mInstanceNames.Add(dynMatName);

						FLinearColor primaryColor = self->mCachedBuildSystem->GetColorSlotPrimary_Linear(i);
						dynMat->SetVectorParameterValue(PrimaryColor, primaryColor);

						FLinearColor secondaryColor = self->mCachedBuildSystem->GetColorSlotSecondary_Linear(i);
						dynMat->SetVectorParameterValue(SecondaryColor, secondaryColor);
					}
				}
			}
			self->mInitialLookupString = lookupName;
			self->mCachedBuildSystem->mFactoryColoredMaterialMap.Emplace(lookupName, self);
			
		}
	});
}