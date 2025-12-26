#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_CallFunction.h"

#include "MagnusNode_MapIdentical.generated.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

UCLASS()
class MAGNUSNODES_API UMagnusNode_MapIdentical : public UK2Node
{
	GENERATED_BODY()

public:

	//================ Node.Configuration																		========

	//———————— Node.Config																							————

	virtual bool IsNodePure() const override { return true; }
	virtual bool ShouldDrawCompact() const override { return true; }

	//———————— Node.Appearance																						————

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetCompactNodeTitle() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual TSharedPtr<SWidget> CreateNodeImage() const override;

	//================ Pin.Management																			========

	//———————— Pin.Registry																							————

	static FName InputMapAPinName() { return FName(TEXT("MapA")); }
	static FName InputMapBPinName() { return FName(TEXT("MapB")); }
	static FName InputTargetMapPinName() { return FName(TEXT("TargetMap")); }
	static FName InputKeysPinName() { return FName(TEXT("Keys")); }
	static FName InputValuesPinName() { return FName(TEXT("Values")); }
	static FName InputArrayAPinName() { return FName(TEXT("ArrayA")); }
	static FName InputArrayBPinName() { return FName(TEXT("ArrayB")); }
	static FName OutputResultPinName() { return FName(TEXT("ReturnValue")); }

	//———————— Pin.Construction																						————

	virtual void AllocateDefaultPins() override;

	//———————— Pin.Notify																							————

	virtual void PostReconstructNode() override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;

	//================ Blueprint.Integration																	========

	//———————— Blueprint.Menu																						————

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	//———————— Blueprint.Compile																					————

	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
