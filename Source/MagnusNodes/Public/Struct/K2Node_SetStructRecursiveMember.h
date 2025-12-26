#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Property/MagnusNodesStruct.h"

// Generated
#include "K2Node_SetStructRecursiveMember.generated.h"

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

UCLASS()
class MAGNUSNODES_API UK2Node_SetStructRecursiveMember : public UK2Node
{
	GENERATED_BODY()

public:

	//================ Node.Configuration																		========

	//———————— Node.Config																							————

	virtual bool IsNodePure() const override { return false; }
	virtual bool ShouldShowNodeProperties() const override { return true; }

	//———————— Node.Appearance																						————

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;

	//================ Node.Properties																			========

	/** 结构体成员引用（支持递归嵌套和多选） */
	UPROPERTY(EditAnywhere, Category = "MemberSelection")
	FStructRecursiveMemberReference StructMemberReference;

	FNodeTextCache CachedTooltip;
	FNodeTextCache CachedNodeTitle;

	//================ Pin.Management																			========

	//———————— Pin.Construction																						————

	virtual void AllocateDefaultPins() override;

	//———————— Pin.Modification																						————

	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual void PostReconstructNode() override;

	//———————— Pin.ValueChange																						————

	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void PinTypeChanged(UEdGraphPin* Pin) override;

	//———————— Pin.Connection																						————

	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;

	//———————— Pin.Reconstruct																						————

	virtual void ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const override;

	//================ Struct.Access																			========

	UScriptStruct* GetStructType() const;
	FProperty* GetPropertyFromPath(const FString& MemberPath) const;
	void SetMemberPaths(const TArray<FString>& InMemberPaths);
	virtual void OnMemberReferenceChanged();

	//================ Pin.Helpers																				========

	static FName StructPinName() { return FName(TEXT("Struct")); }
	static FName InputPinName(const FString& MemberPath);

	//================ Editor.PropertyChange																	========

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	//================ Blueprint.Integration																	========

	//———————— Blueprint.Menu																						————

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	//———————— Blueprint.Compile																					————

	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
