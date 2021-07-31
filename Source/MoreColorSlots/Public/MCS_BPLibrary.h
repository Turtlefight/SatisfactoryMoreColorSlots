#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MCS_BPLibrary.generated.h"


UCLASS()
class MORECOLORSLOTS_API UMCS_BPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MCS", meta=(WorldContext="WorldContextObject"))
	static void ResetColorSlotsToVanilla(UObject* WorldContextObject);
};