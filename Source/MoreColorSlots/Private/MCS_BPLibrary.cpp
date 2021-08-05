
#include "MCS_BPLibrary.h"

#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"

#include "FGColorInterface.h"
#include "FGBuildableSubsystem.h"
#include "FGColorGun.h"

#undef TEXT
#include "Windows/MinWindows.h"
#include "commdlg.h"
#undef UpdateResource


uint8 UMCS_BPLibrary::GetNumberOfColorSlots(EMCS_AdditionalColorPalettes additionalPalettes) {
	uint8 PaletteColorSlots = 18;
	switch (additionalPalettes) {
	default:
	case EMCS_AdditionalColorPalettes::VanillaOnly:
		return PaletteColorSlots * 1;
	case EMCS_AdditionalColorPalettes::Plus1:
		return PaletteColorSlots * 2;
	case EMCS_AdditionalColorPalettes::Plus2:
		return PaletteColorSlots * 3;
	case EMCS_AdditionalColorPalettes::Plus4:
		return PaletteColorSlots * 5;
	case EMCS_AdditionalColorPalettes::Plus6:
		return PaletteColorSlots * 7;
	case EMCS_AdditionalColorPalettes::Plus8:
		return PaletteColorSlots * 9;
	case EMCS_AdditionalColorPalettes::Plus10:
		return PaletteColorSlots * 11;
	case EMCS_AdditionalColorPalettes::Plus12:
		return PaletteColorSlots * 13;
	case EMCS_AdditionalColorPalettes::Plus14:
		return 254;
	}
}

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


typedef TSharedRef< TJsonWriter< TCHAR, TPrettyJsonPrintPolicy< TCHAR > > > FPrettyJsonWriter;
typedef TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>> FPrettyJsonWriterFactory;

static FString MCS_GetSaveFileName(FString const& paletteName) {
	OPENFILENAMEW openFileName;
	wchar_t* fileNameBuffer = (wchar_t*)malloc(sizeof(wchar_t) * 4096);
	wcscpy_s(fileNameBuffer, 4096, *paletteName);
	wcscat_s(fileNameBuffer, 4096, L".json");

	ZeroMemory(&openFileName, sizeof(openFileName));
	openFileName.lStructSize = sizeof(openFileName);
	openFileName.lpstrFilter = L"Color Palette (*.json)\0*.json\0All Files\0*.*\0\0";
	openFileName.lpstrFile = fileNameBuffer;
	openFileName.nMaxFile = 4096;
	openFileName.Flags = OFN_OVERWRITEPROMPT;

	BOOL success = GetSaveFileNameW(&openFileName);
	FString saveName;
	if (success) saveName = fileNameBuffer;
	free(fileNameBuffer);

	return saveName;
}

static FString MCS_GetOpenFileName() {
	OPENFILENAMEW openFileName;
	wchar_t* fileNameBuffer = (wchar_t*)malloc(sizeof(wchar_t) * 4096);
	fileNameBuffer[0] = L'\0';

	ZeroMemory(&openFileName, sizeof(openFileName));
	openFileName.lStructSize = sizeof(openFileName);
	openFileName.lpstrFilter = L"Color Palette (*.json)\0*.json\0All Files\0*.*\0\0";
	openFileName.lpstrFile = fileNameBuffer;
	openFileName.nMaxFile = 4096;

	BOOL success = GetOpenFileNameW(&openFileName);
	FString openName;
	if (success) openName = fileNameBuffer;
	free(fileNameBuffer);

	return openName;
}

static TSharedPtr<FJsonObject> MCS_ColorToJson(FLinearColor color) {
	TSharedPtr<FJsonObject> entry = MakeShareable(new FJsonObject());
	entry->SetNumberField(TEXT("r"), color.R);
	entry->SetNumberField(TEXT("g"), color.G);
	entry->SetNumberField(TEXT("b"), color.B);
	return entry;
}

static bool MCS_JsonToColor(TSharedPtr<FJsonObject> jsonObject, FLinearColor& color) {
	double value;
	if (!jsonObject->TryGetNumberField(TEXT("r"), value))
		return false;
	color.R = value;
	if (!jsonObject->TryGetNumberField(TEXT("g"), value))
		return false;
	color.G = value;
	if (!jsonObject->TryGetNumberField(TEXT("b"), value))
		return false;
	color.B = value;

	color.A = 1.0f;
	return true;
}

static TSharedPtr<FJsonObject> MCS_ColorSlotToJson(FLinearColor primaryColor, FLinearColor secondaryColor) {
	TSharedPtr<FJsonObject> entry = MakeShareable(new FJsonObject());
	entry->SetObjectField(TEXT("primary"), MCS_ColorToJson(primaryColor));
	entry->SetObjectField(TEXT("secondary"), MCS_ColorToJson(secondaryColor));
	return entry;
}

static bool MCS_JsonToColorSlot(TSharedPtr<FJsonObject> jsonObject, FLinearColor& primaryColor, FLinearColor& secondaryColor) {
	const TSharedPtr<FJsonObject>* entry;
	if (!jsonObject->TryGetObjectField(TEXT("primary"), entry) || !entry)
		return false;
	if (!MCS_JsonToColor(*entry, primaryColor))
		return false;

	if (!jsonObject->TryGetObjectField(TEXT("secondary"), entry) || !entry)
		return false;
	if (!MCS_JsonToColor(*entry, secondaryColor))
		return false;

	return true;
}

static FString MCS_ColorPaletteToJsonString(AFGColorGun* colorGun, FMCS_ColorPaletteEntry const& palette) {
	TSharedPtr<FJsonObject> jsonPalette = MakeShareable(new FJsonObject);
	jsonPalette->SetStringField(TEXT("name"), palette.Name);

	TArray<TSharedPtr<FJsonValue>> colors;
	for (int i = 0; i < palette.SlotCount; i++) {
		uint8 slotIndex = palette.StartIndex + i;
		FLinearColor primaryColor = colorGun->GetPrimaryColorForSlot(slotIndex);
		FLinearColor secondaryColor = colorGun->GetSecondaryColorForSlot(slotIndex);
		TSharedPtr<FJsonValueObject> value = MakeShareable(new FJsonValueObject(MCS_ColorSlotToJson(primaryColor, secondaryColor)));
		colors.Add(value);
	}

	if (palette.SlotCount < 18) {
		for (int i = 0; i < (18 - palette.SlotCount); i++) {
			uint8 slotIndex = palette.StartIndex + i;
			FLinearColor primaryColor(FColor(250, 149, 73));
			FLinearColor secondaryColor(FColor(95, 102, 140));
			TSharedPtr<FJsonValueObject> value = MakeShareable(new FJsonValueObject(MCS_ColorSlotToJson(primaryColor, secondaryColor)));
			colors.Add(value);
		}
	}

	jsonPalette->SetArrayField(TEXT("colors"), colors);

	FString jsonStr;
	FPrettyJsonWriter jsonWriter = FPrettyJsonWriterFactory::Create(&jsonStr);
	FJsonSerializer::Serialize(jsonPalette.ToSharedRef(), jsonWriter);

	return jsonStr;
}

static bool MCS_JsonStringToColorPalette(FString const& jsonStr, FString& paletteName, TArray<FLinearColor>& primaryColors, TArray<FLinearColor>& secondaryColors) {
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(jsonStr);
	TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject);
	if (!FJsonSerializer::Deserialize(reader, jsonObject))
		return false;

	if (!jsonObject->TryGetStringField(TEXT("name"), paletteName))
		return false;

	const TArray<TSharedPtr<FJsonValue>>* colors;
	if (!jsonObject->TryGetArrayField(TEXT("colors"), colors) || !colors)
		return false;
	if (colors->Num() != 18)
		return false;

	for (int i = 0, len = colors->Num(); i < len; i++) {
		const TSharedPtr<FJsonObject>* colorEntry;
		if (!(*colors)[i]->TryGetObject(colorEntry) || !colorEntry)
			return false;

		FLinearColor primaryColor;
		FLinearColor secondaryColor;
		if (!MCS_JsonToColorSlot(*colorEntry, primaryColor, secondaryColor))
			return false;

		primaryColors.Add(primaryColor);
		secondaryColors.Add(secondaryColor);
	}

	return true;
}

static void MCS_ApplyColorData(
	AFGColorGun* colorGun,
	uint8 startIndex,
	uint8 slotCount,
	TArray<FLinearColor> const& primaryColors,
	TArray<FLinearColor> const& secondaryColors
) {
	for (uint8 i = 0; i < slotCount; i++) {
		uint8 slotIndex = startIndex + i;
		colorGun->SetPrimaryColorForSlot(slotIndex, primaryColors[i]);
		colorGun->SetSecondaryColorForSlot(slotIndex, secondaryColors[i]);
	}
}

bool UMCS_BPLibrary::SaveColorPaletteToDisk(AFGColorGun* colorGun, FMCS_ColorPaletteEntry const& palette) {
	check(colorGun);

	FString saveFileName = MCS_GetSaveFileName(palette.Name);
	if (saveFileName.IsEmpty()) return false;

	FString jsonStr = MCS_ColorPaletteToJsonString(colorGun, palette);

	if (!FFileHelper::SaveStringToFile(jsonStr, *saveFileName))
		return false;

	return true;
}

bool UMCS_BPLibrary::LoadColorPaletteFromDisk(AFGColorGun* colorGun, FMCS_ColorPaletteEntry const& palette, FString& newPaletteName) {
	check(colorGun);

	FString openFileName = MCS_GetOpenFileName();
	if (openFileName.IsEmpty()) return false;

	FString jsonStr;
	if (!FFileHelper::LoadFileToString(jsonStr, *openFileName))
		return false;

	FString paletteName;
	TArray<FLinearColor> primaryColors;
	TArray<FLinearColor> secondaryColors;
	if (!MCS_JsonStringToColorPalette(jsonStr, paletteName, primaryColors, secondaryColors))
		return false;

	MCS_ApplyColorData(colorGun, palette.StartIndex, palette.SlotCount, primaryColors, secondaryColors);
	newPaletteName = paletteName;

	return true;
}
