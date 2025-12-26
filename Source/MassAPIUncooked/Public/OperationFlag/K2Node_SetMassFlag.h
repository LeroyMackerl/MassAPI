// Leroy Works & Ember, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "MassAPIEnums.h"

#include "K2Node_SetMassFlag.generated.h"

class FBlueprintActionDatabaseRegistrar;
class FKismetCompilerContext;
class UEdGraph;
class UEdGraphPin;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

UCLASS()
class MASSAPIUNCOOKED_API UK2Node_SetMassFlag : public UK2Node
{
	GENERATED_BODY()

public:

	//================ Node.Configuration																		========	

	//———————— Node.Config																							————

	virtual bool IsNodePure() const override { return false; }
	virtual bool ShouldShowNodeProperties() const override { return true; }

	//———————— Node.Appearance																						————

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetMenuCategory() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual FLinearColor GetNodeTitleColor() const override;

	//================ Pin.Management																			========

	//———————— Pin.Construction																						————

	virtual void AllocateDefaultPins() override;

	//———————— Pin.Modification																						————

	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual void PostReconstructNode() override;

	//———————— Pin.Connection																						————

	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;

	//================ DataSource.Access																		========

	// 缓存的DataSource类型
	EMassFragmentSourceDataType CachedDataSourceType = EMassFragmentSourceDataType::None;

	// 更新DataSource类型（从引脚类型推断）
	void UpdateDataSourceType();

	//================ Editor.PropertyChange																	========

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	//================ Blueprint.Integration																	========

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	//================ Compiler.Integration																		========

	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	//================ Pin.Names																				========

	static FName DataSourcePinName() { return TEXT("DataSource"); }
	static FName FlagPinName() { return TEXT("Flag"); }
	static FName ReturnValuePinName() { return TEXT("ReturnValue"); }

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
