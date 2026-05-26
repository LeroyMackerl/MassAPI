/*
* MassAPI
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_MakeLiteralFlagName.generated.h"

class FBlueprintActionDatabaseRegistrar;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/**
 * Pure node that outputs a single FName literal picked from the FlagRegistry dropdown.
 * Follows UK2Node_EnumLiteral pattern: input pin (with dropdown) → output pin.
 * | 纯节点，从下拉列表选择一个旗标名，输出 FName。仿 UK2Node_EnumLiteral 模式。
 */
UCLASS()
class MASSAPIUNCOOKED_API UK2Node_MakeLiteralFlagName : public UK2Node
{
	GENERATED_BODY()

public:

	//================ Node.Configuration																		========

	virtual bool IsNodePure() const override { return true; }
	virtual bool ShouldShowNodeProperties() const override { return true; }

	//———————— Node.Appearance																						————

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetMenuCategory() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual FLinearColor GetNodeTitleColor() const override;

	//================ Pin.Management																			========

	virtual void AllocateDefaultPins() override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;

	//================ Blueprint.Integration																	========

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	//================ Compiler.Integration																		========

	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	//================ Pin.Names																				========

	static FName ValuePinName() { return TEXT("Value"); }

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
