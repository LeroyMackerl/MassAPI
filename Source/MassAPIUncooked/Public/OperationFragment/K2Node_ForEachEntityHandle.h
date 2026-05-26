/*
* MassAPI
* Created: 2026
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#pragma once

#include "K2Node.h"
#include "K2Node_ForEachEntityHandle.generated.h"

UCLASS()
class MASSAPIUNCOOKED_API UK2Node_ForEachEntityHandle : public UK2Node
{
	GENERATED_BODY()

public:

	//================ Node.Configuration ========

	virtual bool IsNodePure() const override { return false; }
	virtual bool ShouldShowNodeProperties() const override { return false; }

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetMenuCategory() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual FLinearColor GetNodeTitleColor() const override;

	//================ Pin.Management ========

	virtual void AllocateDefaultPins() override;

	//================ Blueprint.Integration ========
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	//================ Compiler.Integration ========
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	//================ Pin.Names ========
	static FName ArrayPinName()     { return TEXT("Array"); }
	static FName LoopBodyPinName()  { return TEXT("LoopBody"); }
	static FName ElementPinName()   { return TEXT("Element"); }
	static FName IndexPinName()     { return TEXT("Index"); }
	static FName CompletedPinName() { return TEXT("Completed"); }
};
