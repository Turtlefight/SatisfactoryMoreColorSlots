
#include "MCS_ColorGunWidgetOverride.h"
#include "MCS_MoreColorSlotsConfigStruct.h"

#include "SML/Public/Patching/BlueprintHookManager.h"
#include "SML/Public/Patching/BlueprintHookHelper.h"
#include "Components/WrapBox.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "FGHUD.h"
#include "UI/FGGameUI.h"


void MCS_SetupColorGunWidgetOverride() {
	UBlueprintHookManager* HookManager = GEngine->GetEngineSubsystem<UBlueprintHookManager>();
	check(HookManager);
	UClass* ColorGunWidgetClass = LoadObject<UClass>(nullptr, TEXT("/Game/FactoryGame/Equipment/ColorGun/Widget_ColorGun_ColorPicker.Widget_ColorGun_ColorPicker_C"));
	check(ColorGunWidgetClass);
	UFunction* ColorGunSetupSlots = ColorGunWidgetClass->FindFunctionByName(TEXT("SetupSlots"));
	check(ColorGunSetupSlots);
	
	HookManager->HookBlueprintFunction(ColorGunSetupSlots, [](FBlueprintHookHelper& helper) {
		UObject* context = helper.GetContext();
		UUserWidget* contextWidget = CastChecked<UUserWidget>(context);

		
		FObjectProperty* mColorGridProperty = CastFieldChecked<FObjectProperty>(context->GetClass()->FindPropertyByName(TEXT("mColorGrid")));
		check(mColorGridProperty);
		UObject* mColorGridObj = mColorGridProperty->GetPropertyValue_InContainer(context, 0);
		UWrapBox* mColorGrid = CastChecked<UWrapBox>(mColorGridObj);
		mColorGrid->WrapWidth = 590.0f;
		check(mColorGrid);
		
		UPanelWidget* containerElem = mColorGrid->GetParent();
		if (!containerElem->IsA(UScrollBox::StaticClass())) {
			UHorizontalBox* container = CastChecked<UHorizontalBox>(containerElem);
			TArray<UPanelSlot*> slots = container->GetSlots();
			TArray< UWidget* > children = container->GetAllChildren();
			container->ClearChildren();
			
			UScrollBox* scrollBox = contextWidget->WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), NAME_None);
			scrollBox->SetWheelScrollMultiplier(scrollBox->WheelScrollMultiplier * 1.5f);
			scrollBox->AddChild(mColorGrid);

			USizeBox* sizeBox = contextWidget->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), NAME_None);
			sizeBox->SetHeightOverride(400);
			sizeBox->AddChild(scrollBox);

			children[0] = sizeBox;
			for (int i = 0, len = children.Num(); i < len; i++) {
				UWidget* widget = children[i];
				UHorizontalBoxSlot* slot = CastChecked<UHorizontalBoxSlot>(slots[i]);

				UHorizontalBoxSlot* newSlot = container->AddChildToHorizontalBox(widget);
				newSlot->SetHorizontalAlignment(slot->HorizontalAlignment);
				newSlot->SetPadding(slot->Padding);
				newSlot->SetSize(slot->Size);
				newSlot->SetVerticalAlignment(slot->VerticalAlignment);
			}
		}
	}, EPredefinedHookOffset::Start);

	
	UClass* EquipColorGunClass = LoadObject<UClass>(nullptr, TEXT("/Game/FactoryGame/Equipment/ColorGun/Equip_ColorGun.Equip_ColorGun_C"));
	check(EquipColorGunClass);
	UFunction* ToggleColorPickerUI = EquipColorGunClass->FindFunctionByName(TEXT("ToggleColorPickerUI"));
	check(ToggleColorPickerUI);

	HookManager->HookBlueprintFunction(ToggleColorPickerUI, [](FBlueprintHookHelper& helper) {
		AActor* context = CastChecked<AActor>(helper.GetContext());
		FMCS_MoreColorSlotsConfigStruct config = FMCS_MoreColorSlotsConfigStruct::GetActiveConfig();

		if (config.EnableBetterColorPicker) {
			FObjectProperty* mColorWidgetProperty = CastFieldChecked<FObjectProperty>(context->GetClass()->FindPropertyByName(TEXT("mColorWidget")));
			check(mColorWidgetProperty);

			UObject* mColorWidget = mColorWidgetProperty->GetPropertyValue_InContainer(context, 0);
			bool valid = mColorWidget && !mColorWidget->IsPendingKill();
			if (valid) {
				UFunction* ClearColorPickerUI = context->GetClass()->FindFunctionByName(TEXT("Event Clear Color Picker UI"));
				check(ClearColorPickerUI);

				context->ProcessEvent(ClearColorPickerUI, nullptr);
			}

			APawn* pawn = context->GetInstigator();
			APlayerController* controller = Cast<APlayerController>(pawn->GetController());
			if (controller) {
				UClass* NewColorGunUIClass = LoadObject<UClass>(nullptr, TEXT("/MoreColorSlots/UI/MCS_InteractWidget_BetterColorGunMenu.MCS_InteractWidget_BetterColorGunMenu_C"));
				check(NewColorGunUIClass);

				UUserWidget* newUI = CreateWidget(controller, NewColorGunUIClass, NAME_None);

				FObjectProperty* mColorGunProperty = CastFieldChecked<FObjectProperty>(NewColorGunUIClass->FindPropertyByName(TEXT("mColorGun")));
				check(mColorGunProperty);
				mColorGunProperty->SetPropertyValue_InContainer(newUI, context, 0);

				mColorWidgetProperty->SetObjectPropertyValue_InContainer(context, newUI, 0);

				AFGHUD* hud = Cast<AFGHUD>(controller->GetHUD());
				if (hud) {
					UFGGameUI* gameUI = hud->GetGameUI();
					gameUI->PushWidget(CastChecked<UFGInteractWidget>(newUI));
				}
			}

			helper.JumpToFunctionReturn();
		}
	}, EPredefinedHookOffset::Start);
}