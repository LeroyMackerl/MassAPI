#pragma once

#include "CoreMinimal.h"
#include "K2Node_FragmentFunction.h"
#include "K2Node_SetFragmentInTemplate.generated.h"

UCLASS()
class MASSAPIUNCOOKED_API UK2Node_SetFragmentInTemplate : public UK2Node_FragmentFunction
{
	GENERATED_BODY()

public:
	UK2Node_SetFragmentInTemplate();

	//~ Begin UEdGraphNode Interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	//~ End UEdGraphNode Interface

	//~ Begin UK2Node Interface
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual bool IsNodePure() const override { return false; }
	//~ End UK2Node Interface

protected:
	//~ Begin UK2Node_FragmentFunction Interface
	virtual void OnFragmentTypeChanged() override;
	virtual bool IsValidFragmentStruct(const UScriptStruct* Struct) const override;
	//~ End UK2Node_FragmentFunction Interface

private:
	/** Name constants for pins */
	static const FName FragmentInPinName;
};
