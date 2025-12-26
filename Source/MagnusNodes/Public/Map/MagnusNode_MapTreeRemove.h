#pragma once

#include "BlueprintActionFilter.h"
#include "BlueprintNodeSignature.h"
#include "CoreMinimal.h"
#include "K2Node.h"
#include "Property/MagnusNodesStruct.h"
#include "FuncLib/MagnusFuncLib_Map.h"

#include "MagnusNode_MapTreeRemove.generated.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

UCLASS()
class MAGNUSNODES_API UMagnusNode_MapTreeRemove : public UK2Node
{
	GENERATED_BODY()

public:

	//================ Node.Configuration		 		 		 		 		 		 		 				========

	//———————— Node.Config		 		 		 		 		 		 		 		 		 					————

	virtual bool IsNodePure() const override { return false; }
	virtual bool ShouldDrawCompact() const override { return true; }
	virtual bool IncludeParentNodeContextMenu() const override { return true; }
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual int32 GetNodeRefreshPriority() const override { return Low_UsesDependentWildcard; }

	//———————— Node.Appearance		 		 		 		 		 		 		 		 		 				————

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetCompactNodeTitle() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual TSharedPtr<SWidget> CreateNodeImage() const override;

	//================ Pin.Management		 		 		 		 		 		 		 		 			========

	//———————— Pin.Registry		 		 		 		 		 		 		 		 		 					————

	static FName InputMapPinName() { return FName(TEXT("TargetMap")); }
	static FName InputKeyPinName() { return FName(TEXT("Key")); }
	static FName InputSubKeyPinName() { return FName(TEXT("SubKey")); }
	static FName InputItemPinName() { return FName(TEXT("Value")); }

	//———————— Pin.Construction		 		 		 		 		 		 		 		 		 				————

	virtual void AllocateDefaultPins() override;

	//———————— Pin.Notify		 		 		 		 		 		 		 		 		 					————

	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void ReconstructNode() override;
	virtual void PostReconstructNode() override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;

	//———————— Pin.Connection		 		 		 		 		 		 		 		 		 				————

	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;

	//———————— Pin.Refresh		 		 		 		 		 		 		 		 		 					————

	void PropagatePinType();
	FEdGraphPinType GetKeyPinType() const;
	FEdGraphPinType GetSubKeyPinType() const;
	FEdGraphPinType GetItemPinType() const;

	//================ Container.Type.Management		 		 		 		 		 		 		 		========

	//———————— Container.Type		 		 		 		 		 		 		 		 		 				————

	UPROPERTY(EditAnywhere, Category = "MapTreeRemove", meta=(PropertyFilter="ContainerOnly"))
	FStructMemberReference MemberReference;

	UPROPERTY()
	EMapTreeContainerType ContainerType = EMapTreeContainerType::NoContainer;

	//———————— Property.Change		 		 		 		 		 		 		 		 		 		 		————

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//================ Blueprint.Integration		 		 		 		 		 		 		 		 	========

	//———————— Blueprint.Menu		 		 		 		 		 		 		 		 		 				————

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	//———————— Blueprint.Compile		 		 		 		 		 		 		 		 		 			————

	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;



};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
