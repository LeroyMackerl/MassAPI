/*
* MassAPI
* Created: 2025
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#include "MassAPIUncooked/Public/OperationTag/K2Node_RemoveMassTag.h"
#include "MassAPIFuncLib.h"
#include "MassAPIStructs.h"
#include "MassEntityTypes.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "NodeCompiler/Magnus_HyperNodeCompilerHandler.h"
#include "K2Node_CallFunction.h"
#include "K2Node_TemporaryVariable.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_IfThenElse.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace UK2Node_RemoveMassTagHelper
{
	static const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,					TEXT("RemoveMassTag") },
		{ EMassFragmentSourceDataType::EntityHandle,			TEXT("RemoveMassTag-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,		TEXT("RemoveMassTag-Template") },
		{ EMassFragmentSourceDataType::EntityHandleArray,		TEXT("RemoveMassTag-Entities") },
		{ EMassFragmentSourceDataType::EntityTemplateDataArray,	TEXT("RemoveMassTag-Templates") }
	};

	// 3. Clear - Icon Colors
	static const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceIconColors =
	{
		{ EMassFragmentSourceDataType::None,                    FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,            FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,      FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandleArray,       FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateDataArray, FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
	};

	// 3. Clear - Title Colors
	static const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceTitleColors =
	{
		{ EMassFragmentSourceDataType::None,                    FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,            FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,      FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandleArray,       FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateDataArray, FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
	};
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace K2Node_RemoveMassTag_Local
{

static void SetDelegatePinType(UEdGraphPin* Pin)
{
	if (!Pin) { return; }

	// Use SetFragment_Entity_Unified from RemoveFragment logic as the reference signature
	// as it contains the definition for FOnMassDeferredFinished.
	UFunction* Function = UMassAPIFuncLib::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UMassAPIFuncLib, SetFragment_Entity_Unified));

	if (Function)
	{
		// Find the specific property "OnFinished"
		if (FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(Function->FindPropertyByName(TEXT("OnFinished"))))
		{
			Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Delegate;
			Pin->PinType.PinSubCategoryObject = DelegateProp->SignatureFunction;

			if (DelegateProp->SignatureFunction)
			{
				FMemberReference::FillSimpleMemberReference(static_cast<UFunction*>(DelegateProp->SignatureFunction.Get()), Pin->PinType.PinSubCategoryMemberReference);
			}
		}
	}
}

} // namespace K2Node_RemoveMassTag_Local

FText UK2Node_RemoveMassTag::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(UK2Node_RemoveMassTagHelper::DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_RemoveMassTag::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Tag"));
}

FText UK2Node_RemoveMassTag::GetTooltipText() const
{
	FString Tooltip = TEXT("Remove a tag from an entity or entity template.");

	if (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray ||
		CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateDataArray)
	{
		Tooltip += TEXT(" (Array mode: processes each element in a loop. For deferred mode, OnFinished fires once per element.)");
	}

	return FText::FromString(Tooltip);
}

FSlateIcon UK2Node_RemoveMassTag::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = UK2Node_RemoveMassTagHelper::DataSourceIconColors.FindRef(CachedDataSourceType);
	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UK2Node_RemoveMassTag::GetNodeTitleColor() const
{
	return UK2Node_RemoveMassTagHelper::DataSourceTitleColors.FindRef(CachedDataSourceType);
}

void UK2Node_RemoveMassTag::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsReference = true;

	UEdGraphPin* TagTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, UScriptStruct::StaticClass(), TagTypePinName());
	TagTypePin->PinType.PinSubCategory = UEdGraphSchema_K2::PSC_Self;

	// Create Deferred Pin
	UEdGraphPin* DeferredPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, DeferredPinName());
	DeferredPin->DefaultValue = TEXT("false");

	Super::AllocateDefaultPins();
	UpdateDataSourceType();
}

void UK2Node_RemoveMassTag::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin && OldPin->PinName == DataSourcePinName())
		{
			if (UEdGraphPin* NewDataSourcePin = FindPin(DataSourcePinName()))
			{
				NewDataSourcePin->PinType = OldPin->PinType;
				NewDataSourcePin->PinType.bIsReference = true;
			}
			break;
		}
	}

	// Restore Deferred / Delegate Logic
	// [FIX] Check OLD pins for the deferred value.
	bool bWasDeferred = false;
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin && OldPin->PinName == DeferredPinName())
		{
			bWasDeferred = (OldPin->DefaultValue == TEXT("true"));
			break;
		}
	}

	if (bWasDeferred)
	{
		UEdGraphPin* DelegatePin = FindPin(OnFinishedPinName());
		if (!DelegatePin)
		{
			DelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, OnFinishedPinName());
		}

		K2Node_RemoveMassTag_Local::SetDelegatePinType(DelegatePin);

		for (UEdGraphPin* OldPin : OldPins)
		{
			if (OldPin && OldPin->PinName == OnFinishedPinName())
			{
				if (OldPin->LinkedTo.Num() > 0)
				{
					for (UEdGraphPin* Linked : OldPin->LinkedTo)
					{
						DelegatePin->MakeLinkTo(Linked);
					}
				}
				break;
			}
		}
	}

	UpdateDataSourceType();

	UScriptStruct* OldTagStruct = GetTagStructFromOldPins(OldPins);
	if (OldTagStruct)
	{
		if (UEdGraphPin* TagTypePin = FindPin(TagTypePinName()))
		{
			TagTypePin->DefaultObject = OldTagStruct;
		}
	}
}

void UK2Node_RemoveMassTag::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
	OnTagTypeChanged();
}

void UK2Node_RemoveMassTag::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
	if (Pin && Pin->PinName == TagTypePinName())
	{
		OnTagTypeChanged();
	}

	// Handle Deferred Checkbox Toggle
	if (Pin && Pin->PinName == DeferredPinName())
	{
		const bool bIsChecked = (Pin->DefaultValue == TEXT("true"));
		UEdGraphPin* DelegatePin = FindPin(OnFinishedPinName());

		bool bChanged = false;

		if (bIsChecked && !DelegatePin)
		{
			// User checked 'bDeferred': Create the pin
			DelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, OnFinishedPinName());
			K2Node_RemoveMassTag_Local::SetDelegatePinType(DelegatePin);
			bChanged = true;
		}
		else if (!bIsChecked && DelegatePin)
		{
			// User unchecked 'bDeferred': Destroy the pin
			DelegatePin->BreakAllPinLinks();
			RemovePin(DelegatePin);
			bChanged = true;
		}

		if (bChanged)
		{
			GetGraph()->NotifyNodeChanged(this);
		}
	}
}

bool UK2Node_RemoveMassTag::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	if (MyPin && MyPin->PinName == DataSourcePinName())
	{
		if (OtherPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			OutReason = TEXT("DataSource cannot connect to Wildcard. It must be either FEntityHandle or FEntityTemplateData.");
			return true;
		}

		if (const UScriptStruct* PinStruct = Cast<UScriptStruct>(OtherPin->PinType.PinSubCategoryObject.Get()))
		{
			if (PinStruct != FEntityHandle::StaticStruct() && PinStruct != FEntityTemplateData::StaticStruct())
			{
				OutReason = TEXT("DataSource must be either FEntityHandle or FEntityTemplateData.");
				return true;
			}
		}
		else
		{
			OutReason = TEXT("DataSource must be either FEntityHandle or FEntityTemplateData.");
			return true;
		}
	}

	if (MyPin && MyPin->PinName == TagTypePinName())
	{
		if (const UScriptStruct* PinStruct = Cast<UScriptStruct>(OtherPin->PinType.PinSubCategoryObject.Get()))
		{
			if (!IsValidTagStruct(PinStruct))
			{
				OutReason = TEXT("The provided struct must be a valid Mass Tag type.");
				return true;
			}
		}
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_RemoveMassTag::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin && Pin->PinName == DataSourcePinName())
	{
		UEdGraphPin* DataSourcePin = FindPin(DataSourcePinName());
		if (DataSourcePin)
		{
			if (DataSourcePin->LinkedTo.Num() > 0)
			{
				UEdGraphPin* LinkedPin = DataSourcePin->LinkedTo[0];
				if (LinkedPin && LinkedPin->PinType.PinSubCategoryObject.IsValid())
				{
					DataSourcePin->PinType = LinkedPin->PinType;
					DataSourcePin->PinType.bIsReference = true;
				}
			}
			else
			{
				DataSourcePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
				DataSourcePin->PinType.PinSubCategoryObject = nullptr;
				DataSourcePin->PinType.bIsReference = true;
			}

			UpdateDataSourceType();
			GetGraph()->NotifyNodeChanged(this);
		}
	}

	if (Pin && Pin->PinName == TagTypePinName())
	{
		if (Pin->LinkedTo.Num() == 0)
		{
			OnTagTypeChanged();
		}
	}
}

void UK2Node_RemoveMassTag::UpdateDataSourceType()
{
	UEdGraphPin* DataSourcePin = FindPin(DataSourcePinName());
	if (!DataSourcePin)
	{
		CachedDataSourceType = EMassFragmentSourceDataType::None;
		return;
	}

	// Extract struct and container types
	UScriptStruct* DataSourceStruct = Cast<UScriptStruct>(DataSourcePin->PinType.PinSubCategoryObject.Get());
	const bool bIsArray = (DataSourcePin->PinType.ContainerType == EPinContainerType::Array);

	// Decision table
	if (DataSourceStruct == FEntityHandle::StaticStruct())
	{
		CachedDataSourceType = bIsArray ?
			EMassFragmentSourceDataType::EntityHandleArray :
			EMassFragmentSourceDataType::EntityHandle;
	}
	else if (DataSourceStruct == FEntityTemplateData::StaticStruct())
	{
		CachedDataSourceType = bIsArray ?
			EMassFragmentSourceDataType::EntityTemplateDataArray :
			EMassFragmentSourceDataType::EntityTemplateData;
	}
	else
	{
		CachedDataSourceType = EMassFragmentSourceDataType::None;
	}

	// Toggle bDeferred visibility based on source type
	// Now support deferred mode for both single and array entity modes
	UEdGraphPin* DeferredPin = FindPin(DeferredPinName());
	if (DeferredPin)
	{
		// Only Entities (single or array) can handle deferred commands
		const bool bShow = (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle ||
							CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray);

		if (DeferredPin->bHidden == bShow)
		{
			DeferredPin->bHidden = !bShow;
			if (!bShow)
			{
				DeferredPin->BreakAllPinLinks();
			}
		}

		// Also handle OnFinished visibility if we are hiding Deferred (e.g. switching to TemplateData mode)
		UEdGraphPin* DelegatePin = FindPin(OnFinishedPinName());
		if (DelegatePin && !bShow)
		{
			DelegatePin->BreakAllPinLinks();
			RemovePin(DelegatePin);
		}
	}
}

void UK2Node_RemoveMassTag::OnTagTypeChanged()
{
	GetGraph()->NotifyNodeChanged(this);
}

bool UK2Node_RemoveMassTag::IsValidTagStruct(const UScriptStruct* Struct) const
{
	if (!Struct)
	{
		return false;
	}

	return Struct->IsChildOf(FMassTag::StaticStruct());
}

UScriptStruct* UK2Node_RemoveMassTag::GetTagStruct() const
{
	UEdGraphPin* TagTypePin = FindPin(TagTypePinName());
	if (TagTypePin && TagTypePin->DefaultObject != nullptr && TagTypePin->LinkedTo.Num() == 0)
	{
		return Cast<UScriptStruct>(TagTypePin->DefaultObject);
	}
	return nullptr;
}

UScriptStruct* UK2Node_RemoveMassTag::GetTagStructFromOldPins(const TArray<UEdGraphPin*>& OldPins) const
{
	const UEdGraphPin* TagTypePin = nullptr;
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin->PinName == TagTypePinName())
		{
			TagTypePin = OldPin;
			break;
		}
	}

	if (TagTypePin && TagTypePin->DefaultObject != nullptr && TagTypePin->LinkedTo.Num() == 0)
	{
		return Cast<UScriptStruct>(TagTypePin->DefaultObject);
	}
	return nullptr;
}

void UK2Node_RemoveMassTag::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UK2Node_RemoveMassTag::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

HNCH_StartExpandNode(UK2Node_RemoveMassTag)

virtual void Compile() override
{
	OwnerNode->UpdateDataSourceType();

	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		CompilerContext.MessageLog.Error(TEXT("DataSource must be connected to either FEntityHandle or FEntityTemplateData. @@"), OwnerNode);
		return;
	}

	UScriptStruct* TagStruct = OwnerNode->GetTagStruct();
	if (!TagStruct)
	{
		CompilerContext.MessageLog.Error(TEXT("TagType must be specified. Please select a valid Mass Tag type. @@"), OwnerNode);
		return;
	}

	// Validate array mode
	bool bIsArray = (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray ||
					 OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateDataArray);

	if (bIsArray)
	{
		UEdGraphPin* DataSourcePin = ProxyPin(UK2Node_RemoveMassTag::DataSourcePinName());
		if (!DataSourcePin || DataSourcePin->LinkedTo.Num() == 0)
		{
			CompilerContext.MessageLog.Error(TEXT("DataSource array must be connected. @@"), OwnerNode);
			return;
		}

		if (DataSourcePin->PinType.ContainerType != EPinContainerType::Array)
		{
			CompilerContext.MessageLog.Error(TEXT("Internal error: DataSource type mismatch (expected array). @@"), OwnerNode);
			return;
		}
	}

	// Array mode handling
	if (bIsArray)
	{
		// ARRAY PATH: Generate ForEach loop
		bool bIsEntityMode = (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray);

		// 1. Create loop counter
		UK2Node_TemporaryVariable* LoopCounterNode = SpawnNode<UK2Node_TemporaryVariable>();
		LoopCounterNode->VariableType.PinCategory = UEdGraphSchema_K2::PC_Int;
		LoopCounterNode->AllocateDefaultPins();
		UEdGraphPin* LoopCounterPin = LoopCounterNode->GetVariablePin();

		// 2. Array Length
		UK2Node_CallFunction* ArrayLengthNode = HNCH_SpawnFunctionNode(UKismetArrayLibrary, Array_Length);
		UEdGraphPin* LengthArrayPin = ArrayLengthNode->FindPinChecked(TEXT("TargetArray"), EGPD_Input);
		LengthArrayPin->PinType = ProxyPin(UK2Node_RemoveMassTag::DataSourcePinName())->PinType;
		CompilerContext.CopyPinLinksToIntermediate(*ProxyPin(UK2Node_RemoveMassTag::DataSourcePinName()), *LengthArrayPin);

		// 3. Counter initialization
		UK2Node_AssignmentStatement* CounterInitNode = SpawnNode<UK2Node_AssignmentStatement>();
		CounterInitNode->AllocateDefaultPins();
		Link(LoopCounterPin, CounterInitNode->GetVariablePin());
		CounterInitNode->GetValuePin()->DefaultValue = TEXT("0");

		// 4. Branch node
		UK2Node_IfThenElse* BranchNode = SpawnNode<UK2Node_IfThenElse>();
		BranchNode->AllocateDefaultPins();

		// 5. Loop condition (counter < length)
		UK2Node_CallFunction* ConditionNode = HNCH_SpawnFunctionNode(UKismetMathLibrary, Less_IntInt);
		Link(LoopCounterPin, ConditionNode->FindPinChecked(TEXT("A")));
		Link(ArrayLengthNode->GetReturnValuePin(), ConditionNode->FindPinChecked(TEXT("B")));
		Link(ConditionNode->GetReturnValuePin(), BranchNode->GetConditionPin());

		// 6. Execution sequence
		UK2Node_ExecutionSequence* SequenceNode = SpawnNode<UK2Node_ExecutionSequence>();
		SequenceNode->AllocateDefaultPins();

		// 7. Array Get (extract element)
		UK2Node_CallFunction* ArrayGetNode = HNCH_SpawnFunctionNode(UKismetArrayLibrary, Array_Get);
		UEdGraphPin* GetArrayPin = ArrayGetNode->FindPinChecked(TEXT("TargetArray"), EGPD_Input);
		GetArrayPin->PinType = ProxyPin(UK2Node_RemoveMassTag::DataSourcePinName())->PinType;
		CompilerContext.CopyPinLinksToIntermediate(*ProxyPin(UK2Node_RemoveMassTag::DataSourcePinName()), *GetArrayPin);
		Link(LoopCounterPin, ArrayGetNode->FindPinChecked(TEXT("Index")));

		// Set array element output pin type (copy from array but change container to None)
		UEdGraphPin* ArrayItemPin = ArrayGetNode->FindPinChecked(TEXT("Item"));
		ArrayItemPin->PinType = ProxyPin(UK2Node_RemoveMassTag::DataSourcePinName())->PinType;
		ArrayItemPin->PinType.ContainerType = EPinContainerType::None;

		// 8. Create RemoveTag function call for each element
		UK2Node_CallFunction* RemoveFunctionNode;

		if (bIsEntityMode)
		{
			RemoveFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, RemoveTag_Entity);
			Link(ArrayItemPin, FunctionInputPin(RemoveFunctionNode, TEXT("EntityHandle")));
			Link(ProxyPin(UK2Node_RemoveMassTag::DeferredPinName()), FunctionInputPin(RemoveFunctionNode, TEXT("bDeferred")));
			if (UEdGraphPin* DelegatePin = ProxyPin(UK2Node_RemoveMassTag::OnFinishedPinName()))
			{
				Link(DelegatePin, FunctionInputPin(RemoveFunctionNode, TEXT("OnFinished")));
			}
		}
		else
		{
			RemoveFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, RemoveTag_Template);
			Link(ArrayItemPin, FunctionInputPin(RemoveFunctionNode, TEXT("TemplateData")));
		}

		Link(ProxyPin(UK2Node_RemoveMassTag::TagTypePinName()), FunctionInputPin(RemoveFunctionNode, TEXT("TagType")));

		// Connect execution flow
		Link(SequenceNode->GetThenPinGivenIndex(0), ExecPin(RemoveFunctionNode));
		Link(ThenPin(RemoveFunctionNode), SequenceNode->GetThenPinGivenIndex(1));

		// 9. Increment counter
		UK2Node_CallFunction* IncrementNode = HNCH_SpawnFunctionNode(UKismetMathLibrary, Add_IntInt);
		Link(LoopCounterPin, IncrementNode->FindPinChecked(TEXT("A")));
		IncrementNode->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("1");

		UK2Node_AssignmentStatement* CounterAssignNode = SpawnNode<UK2Node_AssignmentStatement>();
		CounterAssignNode->AllocateDefaultPins();
		Link(LoopCounterPin, CounterAssignNode->GetVariablePin());
		Link(IncrementNode->GetReturnValuePin(), CounterAssignNode->GetValuePin());

		// 10. Connect execution flow
		Link(ProxyExecPin(), ExecPin(CounterInitNode));
		Link(ThenPin(CounterInitNode), ExecPin(BranchNode));
		Link(BranchNode->GetThenPin(), ExecPin(SequenceNode));
		Link(SequenceNode->GetThenPinGivenIndex(1), ExecPin(CounterAssignNode));
		Link(ThenPin(CounterAssignNode), ExecPin(BranchNode));  // Loop back
		Link(BranchNode->GetElsePin(), ProxyThenPin());

		// 11. Reconstruct array function nodes to finalize pin types
		ArrayLengthNode->PostReconstructNode();
		ArrayGetNode->PostReconstructNode();
	}
	else
	{
		// SINGLE-ENTITY/TEMPLATE PATH: Direct function call
		UK2Node_CallFunction* RemoveFunctionNode = nullptr;
		FString FunctionDataSourcePinName;

		switch (OwnerNode->CachedDataSourceType)
		{
		case EMassFragmentSourceDataType::EntityHandle:
			RemoveFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, RemoveTag_Entity);
			FunctionDataSourcePinName = TEXT("EntityHandle");

			// Link bDeferred
			Link(ProxyPin(UK2Node_RemoveMassTag::DeferredPinName()), FunctionInputPin(RemoveFunctionNode, TEXT("bDeferred")));

			// Connect OnFinished Delegate
			if (UEdGraphPin* DelegatePin = ProxyPin(UK2Node_RemoveMassTag::OnFinishedPinName()))
			{
				Link(DelegatePin, FunctionInputPin(RemoveFunctionNode, TEXT("OnFinished")));
			}
			break;

		case EMassFragmentSourceDataType::EntityTemplateData:
			RemoveFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, RemoveTag_Template);
			FunctionDataSourcePinName = TEXT("TemplateData");
			break;
		default:
			return;
		}

		Link(ProxyExecPin(), ExecPin(RemoveFunctionNode));
		Link(ThenPin(RemoveFunctionNode), ProxyThenPin());

		Link(ProxyPin(UK2Node_RemoveMassTag::DataSourcePinName()), FunctionInputPin(RemoveFunctionNode, FunctionDataSourcePinName));
		Link(ProxyPin(UK2Node_RemoveMassTag::TagTypePinName()), FunctionInputPin(RemoveFunctionNode, TEXT("TagType")));
	}
}

HNCH_EndExpandNode(UK2Node_RemoveMassTag)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————