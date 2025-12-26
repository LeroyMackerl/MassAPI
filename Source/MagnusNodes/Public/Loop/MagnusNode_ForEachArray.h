#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"

#include "MagnusNode_ForEachArray.generated.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

UCLASS()
class MAGNUSNODES_API UMagnusNode_ForEachArray : public UK2Node
{
	GENERATED_BODY()

public:

	//================ Node.Configuration																		========

	//———————— Node.Config																							————

	virtual bool IsNodePure() const override { return false; }
	virtual bool ShouldShowNodeProperties() const override { return true; }

	//———————— Node.Appearance																						————

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual FLinearColor GetNodeTitleColor() const override;

	//================ Node.Properties																			========

	UPROPERTY(EditAnywhere, Category="LoopConfig")
	bool CouldBreak = false;

	UPROPERTY(EditAnywhere, Category="LoopConfig")
	bool Reverse = false;

	UPROPERTY(EditAnywhere, Category="NodeTitle", DisplayName="NodeTitle")
	FText NodeTitleText = FText::FromString(TEXT("Default"));

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	//================ Pin.Management																			========

	//———————— Pin.Registry																							————

	UEdGraphPin* GetLoopBodyPin() const;
	UEdGraphPin* GetBreakPin() const;

	UEdGraphPin* GetArrayPin() const;
	UEdGraphPin* GetValuePin() const;
	UEdGraphPin* GetIndexPin() const;

	UEdGraphPin* GetCompletedPin() const;

	static const FName LoopBodyPinName;
	static const FName BreakPinName;

	static const FName ArrayPinName;
	static const FName ValuePinName;
	static const FName IndexPinName;

	//———————— Pin.Construction																						————

	virtual void AllocateDefaultPins() override;

	//———————— Pin.Notify																							————

	virtual void PostReconstructNode() override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;

	//———————— Pin.Refresh																							————

	void RefreshBreakPin() const;
	void RefreshPinType() const;

	//———————— Pin.Connection																						————

	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;

	//================ Blueprint.Integration																	========

	//———————— Blueprint.Menu																						————

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	//———————— Blueprint.Compile																					————

	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————