/*
* MassAPI
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#include "OperationFlag/K2Node_MakeLiteralFlagName.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "KismetCompiler.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Appearance																					========

FText UK2Node_MakeLiteralFlagName::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("MassAPI", "MakeLiteralFlagName", "Make Literal Flag");
}

FText UK2Node_MakeLiteralFlagName::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Flag"));
}

FText UK2Node_MakeLiteralFlagName::GetTooltipText() const
{
	return NSLOCTEXT("MassAPI", "MakeLiteralFlagNameTooltip", "Output a flag name literal (FName) picked from the Flag Registry dropdown. | 输出从下拉选择的旗标名。");
}

FSlateIcon UK2Node_MakeLiteralFlagName::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);
	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UK2Node_MakeLiteralFlagName::GetNodeTitleColor() const
{
	return FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);
}

//================ Pin.Management																					========

void UK2Node_MakeLiteralFlagName::AllocateDefaultPins()
{
	// Input pin: FName, dropdown comes from pin factory | 输入引脚：FName，下拉由引脚工厂提供
	UEdGraphPin* InputPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, NAME_None, ValuePinName());
	InputPin->DefaultValue = TEXT("None");

	// Output pin: FName, pass-through | 输出引脚：FName，透传
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Name, NAME_None, UEdGraphSchema_K2::PN_ReturnValue);

	Super::AllocateDefaultPins();
}

void UK2Node_MakeLiteralFlagName::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin && OldPin->PinName == ValuePinName())
		{
			if (UEdGraphPin* NewPin = FindPin(ValuePinName()))
			{
				NewPin->PinType = OldPin->PinType;
			}
			break;
		}
	}
}

//================ Blueprint.Integration																			========

void UK2Node_MakeLiteralFlagName::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

//================ Compiler.Integration																			========

void UK2Node_MakeLiteralFlagName::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UEdGraphPin* InputPin = FindPinChecked(ValuePinName());
	UEdGraphPin* OutputPin = FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue);

	if (InputPin && OutputPin && OutputPin->LinkedTo.Num() > 0)
	{
		// Move input connections to a literal FName → wire to output | 将输入端 DefaultValue 字面量连到输出
		if (!InputPin->LinkedTo.Num() && !InputPin->DefaultValue.IsEmpty() && InputPin->DefaultValue != TEXT("None"))
		{
			// The pin factory already wrote the selected FName into DefaultValue.
			// We use MovePinLinksToIntermediate to forward it as a compile-time literal. |
			// 引脚工厂已经将选中的 FName 写入了 DefaultValue，编译时作为字面量输出。
			OutputPin->DefaultValue = InputPin->DefaultValue;
		}

		// If the input pin is connected, pass through directly | 如果输入引脚有连接，直接透传
		if (InputPin->LinkedTo.Num() > 0)
		{
			CompilerContext.MovePinLinksToIntermediate(*InputPin, *OutputPin);
		}
	}

	BreakAllNodeLinks();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
