
#include "MCS_ColorGunWidgetOverride.h"

#include "SML/Public/Patching/BlueprintHookManager.h"
#include "SML/Public/Patching/BlueprintHookHelper.h"
#include "Components/WrapBox.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"


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
}