#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Property/MagnusNodesStruct.h"

// Generated
#include "K2Node_GetStructMember.generated.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

UCLASS()
class MAGNUSNODES_API UK2Node_GetStructMember : public UK2Node
{
	GENERATED_BODY()

public:

	//================ Node.Configuration																		========

	//———————— Node.Config																							————

	virtual bool IsNodePure() const override { return true; }

	//================ Node.Properties																			========

	/** 结构体成员引用（仅第一级成员，如 "Translation"、"Rotation"） */
	UPROPERTY(EditAnywhere, Category = "MemberSelection")
	FStructMemberReference StructMemberReference;

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
	FProperty* GetPropertyFromMemberReference() const;
	void SetMemberPath(const FString& InMemberPath);
	virtual void OnMemberReferenceChanged();

	//================ Pin.Helpers																				========

	static FName StructPinName() { return FName(TEXT("Struct")); }
	static FName MemberPinName() { return FName(TEXT("FieldValue")); }

	//================ Editor.PropertyChange																	========

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	//================ Blueprint.Integration																	========

	//———————— Blueprint.Compile																					————

	virtual FNodeHandlingFunctor* CreateNodeHandler(FKismetCompilerContext& CompilerContext) const override;

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
