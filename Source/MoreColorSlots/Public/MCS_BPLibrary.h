#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MCS_BPLibrary.generated.h"

USTRUCT(BlueprintType, Category="MCS")
struct FMCS_ColorPaletteEntry {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MCS")
	uint8 StartIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MCS")
	uint8 SlotCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MCS")
	bool IsVanillaPalette;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MCS")
	FString Name;
};

UENUM(BlueprintType, Category = "MCS")
enum class EMCS_AdditionalColorPalettes : uint8 {
	VanillaOnly UMETA(DisplayName="Vanilla Only"),
	Plus1 UMETA(DisplayName = "+ 1 palette"),
	Plus2 UMETA(DisplayName = "+ 2 palettes"),
	Plus4 UMETA(DisplayName = "+ 4 palettes"),
	Plus6 UMETA(DisplayName = "+ 6 palettes"),
	Plus8 UMETA(DisplayName = "+ 8 palettes"),
	Plus10 UMETA(DisplayName = "+ 10 palettes"),
	Plus12 UMETA(DisplayName = "+ 12 palettes"),
	Plus14 UMETA(DisplayName = "+ 14 palettes"),
	Max UMETA(DisplayName="Max", Hidden)
};

UCLASS()
class MORECOLORSLOTS_API UMCS_BPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "MCS")
	static uint8 GetNumberOfColorSlots(EMCS_AdditionalColorPalettes additionalPalettes);

	UFUNCTION(BlueprintCallable, Category = "MCS", meta=(WorldContext="WorldContextObject"))
	static void ResetColorSlotsToVanilla(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "MCS")
	static bool SaveColorPaletteToDisk(class AFGColorGun* colorGun, FMCS_ColorPaletteEntry const& palette);

	UFUNCTION(BlueprintCallable, Category = "MCS")
	static bool LoadColorPaletteFromDisk(AFGColorGun* colorGun, FMCS_ColorPaletteEntry const& palette, FString& newPaletteName);
};