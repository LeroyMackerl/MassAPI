#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "EdGraph/EdGraphNodeUtils.h"

// Generated
#include "MagnusNode_Assign.generated.h"

class FBlueprintActionDatabaseRegistrar;
class UEdGraphPin;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

UCLASS()
class MAGNUSNODES_API UMagnusNode_Assign : public UK2Node
{
	GENERATED_UCLASS_BODY()

public:

	//================ Node.Configuration																		========

	//———————— Node.Config																							————

	virtual bool ShouldDrawCompact() const { return true; }
	virtual int32 GetNodeRefreshPriority() const override { return EBaseNodeRefreshPriority::Low_UsesDependentWildcard; }

	//———————— Node.Appearance																						————

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetCompactNodeTitle() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;

	//================ Pin.Management																			========

	//———————— Pin.Registry																							————

	UEdGraphPin* GetTargetPin() const;
	UEdGraphPin* GetValuePin() const;

	//———————— Pin.Construction																						————

	virtual void AllocateDefaultPins() override;

	//———————— Pin.Notify																							————

	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PostPasteNode() override;

	//———————— Pin.Refresh																							————

	void CoerceTypeFromPin(const UEdGraphPin* Pin);

	//================ Blueprint.Integration																	========

	//———————— Blueprint.Menu																						————

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override;

	//———————— Blueprint.Compile																					————

	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const override;

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
