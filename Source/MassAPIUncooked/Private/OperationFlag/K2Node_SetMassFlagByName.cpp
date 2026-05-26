/*
* MassAPI
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#include "OperationFlag/K2Node_SetMassFlagByName.h"
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

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
namespace K2Node_SetMassFlagByName_Local
{

static void SetDelegatePinType(UEdGraphPin* Pin)
{
	if (!Pin) { return; }
	UFunction* Function = UMassAPIFuncLib::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UMassAPIFuncLib, SetFragment_Entity_Unified));
	if (Function)
	{
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

} // namespace K2Node_SetMassFlagByName_Local

namespace UK2Node_SetMassFlagByNameHelper
{
	static const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,					TEXT("SetMassFlag") },
		{ EMassFragmentSourceDataType::EntityHandle,			TEXT("SetMassFlag-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,		TEXT("SetMassFlag-Template") },
		{ EMassFragmentSourceDataType::EntityHandleArray,		TEXT("SetMassFlag-Entities") },
		{ EMassFragmentSourceDataType::EntityTemplateDataArray,	TEXT("SetMassFlag-Templates") }
	};

	static const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceIconColors =
	{
		{ EMassFragmentSourceDataType::None,                    FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,            FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,      FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandleArray,       FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateDataArray, FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
	};

	static const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceTitleColors =
	{
		{ EMassFragmentSourceDataType::None,                    FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,            FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,      FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandleArray,       FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateDataArray, FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
	};
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration																			========

//———————— Node.Appearance																							————

FText UK2Node_SetMassFlagByName::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(UK2Node_SetMassFlagByNameHelper::DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_SetMassFlagByName::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Flag"));
}

FText UK2Node_SetMassFlagByName::GetTooltipText() const
{
	FString Tooltip = TEXT("Set a named flag (FName) on an entity or entity template. Resolves via Project Settings. | 按名称设置旗标。");

	if (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray ||
		CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateDataArray)
	{
		Tooltip += TEXT(" (Array mode: processes each element in a loop.)");
	}

	return FText::FromString(Tooltip);
}

FSlateIcon UK2Node_SetMassFlagByName::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = UK2Node_SetMassFlagByNameHelper::DataSourceIconColors.FindRef(CachedDataSourceType);

	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UK2Node_SetMassFlagByName::GetNodeTitleColor() const
{
	return UK2Node_SetMassFlagByNameHelper::DataSourceTitleColors.FindRef(CachedDataSourceType);
}

//================ Pin.Management																				========

//———————— Pin.Construction																							————

void UK2Node_SetMassFlagByName::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// DataSource wildcard pin | DataSource 通配符引脚
	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsReference = true;

	// Flag pin — FName instead of EEntityFlags enum | FName 类型旗标引脚
	UEdGraphPin* FlagPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, NAME_None, FlagPinName());

	// ReturnValue pin (bool, EntityHandle mode only) | 返回值引脚（仅 EntityHandle 模式）
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());

	// Deferred pin (default false) | 延迟标志引脚（默认关闭）
	UEdGraphPin* DeferredPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, DeferredPinName());
	DeferredPin->DefaultValue = TEXT("false");

	Super::AllocateDefaultPins();

	UpdateDataSourceType();
}

//———————— Pin.Modification																							————

void UK2Node_SetMassFlagByName::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
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

		K2Node_SetMassFlagByName_Local::SetDelegatePinType(DelegatePin);

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
}

void UK2Node_SetMassFlagByName::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
}

//------------------------ Pin.ValueChange ------------------------ | 引脚值变更 ------------------------

void UK2Node_SetMassFlagByName::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	if (Pin && Pin->PinName == DeferredPinName())
	{
		const bool bIsChecked = (Pin->DefaultValue == TEXT("true"));
		UEdGraphPin* DelegatePin = FindPin(OnFinishedPinName());

		bool bChanged = false;

		if (bIsChecked && !DelegatePin)
		{
			DelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, OnFinishedPinName());
			K2Node_SetMassFlagByName_Local::SetDelegatePinType(DelegatePin);
			bChanged = true;
		}
		else if (!bIsChecked && DelegatePin)
		{
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

//———————— Pin.Connection																							————

bool UK2Node_SetMassFlagByName::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
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

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_SetMassFlagByName::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
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
}

//================ DataSource.Access			========

void UK2Node_SetMassFlagByName::UpdateDataSourceType()
{
	UEdGraphPin* DataSourcePin = FindPin(DataSourcePinName());
	if (!DataSourcePin)
	{
		CachedDataSourceType = EMassFragmentSourceDataType::None;
		return;
	}

	UScriptStruct* DataSourceStruct = Cast<UScriptStruct>(DataSourcePin->PinType.PinSubCategoryObject.Get());
	const bool bIsArray = (DataSourcePin->PinType.ContainerType == EPinContainerType::Array);

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

	UEdGraphPin* ReturnPin = FindPin(ReturnValuePinName());

	if (CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateData ||
		CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateDataArray ||
		CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray)
	{
		if (ReturnPin)
		{
			ReturnPin->BreakAllPinLinks();
			Pins.Remove(ReturnPin);
		}
	}
	else if (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle)
	{
		if (!ReturnPin)
		{
			CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());
		}
	}
	else if (CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		if (!ReturnPin)
		{
			CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());
		}
	}

	UEdGraphPin* DeferredPin = FindPin(DeferredPinName());
	if (DeferredPin)
	{
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

		UEdGraphPin* DelegatePin = FindPin(OnFinishedPinName());
		if (DelegatePin && !bShow)
		{
			DelegatePin->BreakAllPinLinks();
			RemovePin(DelegatePin);
		}
	}
}

//================ Editor.PropertyChange		========

void UK2Node_SetMassFlagByName::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

//================ Blueprint.Integration																		========

void UK2Node_SetMassFlagByName::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

//================ Compiler.Integration																			========

HNCH_StartExpandNode(UK2Node_SetMassFlagByName)

virtual void Compile() override
{
	OwnerNode->UpdateDataSourceType();

	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		CompilerContext.MessageLog.Error(TEXT("DataSource must be connected to either FEntityHandle or FEntityTemplateData. @@"), OwnerNode);
		return;
	}

	bool bIsArray = (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray ||
					 OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateDataArray);

	if (bIsArray)
	{
		UEdGraphPin* DataSourcePin = ProxyPin(UK2Node_SetMassFlagByName::DataSourcePinName());
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

	if (bIsArray)
	{
		// ARRAY PATH: Generate ForEach loop | 数组路径：生成 ForEach 循环
		bool bIsEntityMode = (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray);

		UK2Node_TemporaryVariable* LoopCounterNode = SpawnNode<UK2Node_TemporaryVariable>();
		LoopCounterNode->VariableType.PinCategory = UEdGraphSchema_K2::PC_Int;
		LoopCounterNode->AllocateDefaultPins();
		UEdGraphPin* LoopCounterPin = LoopCounterNode->GetVariablePin();

		UK2Node_CallFunction* ArrayLengthNode = HNCH_SpawnFunctionNode(UKismetArrayLibrary, Array_Length);
		UEdGraphPin* LengthArrayPin = ArrayLengthNode->FindPinChecked(TEXT("TargetArray"), EGPD_Input);
		LengthArrayPin->PinType = ProxyPin(UK2Node_SetMassFlagByName::DataSourcePinName())->PinType;
		CompilerContext.CopyPinLinksToIntermediate(*ProxyPin(UK2Node_SetMassFlagByName::DataSourcePinName()), *LengthArrayPin);

		UK2Node_AssignmentStatement* CounterInitNode = SpawnNode<UK2Node_AssignmentStatement>();
		CounterInitNode->AllocateDefaultPins();
		Link(LoopCounterPin, CounterInitNode->GetVariablePin());
		CounterInitNode->GetValuePin()->DefaultValue = TEXT("0");

		UK2Node_IfThenElse* BranchNode = SpawnNode<UK2Node_IfThenElse>();
		BranchNode->AllocateDefaultPins();

		UK2Node_CallFunction* ConditionNode = HNCH_SpawnFunctionNode(UKismetMathLibrary, Less_IntInt);
		Link(LoopCounterPin, ConditionNode->FindPinChecked(TEXT("A")));
		Link(ArrayLengthNode->GetReturnValuePin(), ConditionNode->FindPinChecked(TEXT("B")));
		Link(ConditionNode->GetReturnValuePin(), BranchNode->GetConditionPin());

		UK2Node_ExecutionSequence* SequenceNode = SpawnNode<UK2Node_ExecutionSequence>();
		SequenceNode->AllocateDefaultPins();

		UK2Node_CallFunction* ArrayGetNode = HNCH_SpawnFunctionNode(UKismetArrayLibrary, Array_Get);
		UEdGraphPin* GetArrayPin = ArrayGetNode->FindPinChecked(TEXT("TargetArray"), EGPD_Input);
		GetArrayPin->PinType = ProxyPin(UK2Node_SetMassFlagByName::DataSourcePinName())->PinType;
		CompilerContext.CopyPinLinksToIntermediate(*ProxyPin(UK2Node_SetMassFlagByName::DataSourcePinName()), *GetArrayPin);
		Link(LoopCounterPin, ArrayGetNode->FindPinChecked(TEXT("Index")));

		UEdGraphPin* ArrayItemPin = ArrayGetNode->FindPinChecked(TEXT("Item"));
		ArrayItemPin->PinType = ProxyPin(UK2Node_SetMassFlagByName::DataSourcePinName())->PinType;
		ArrayItemPin->PinType.ContainerType = EPinContainerType::None;

		UK2Node_CallFunction* SetFunctionNode;

		if (bIsEntityMode)
		{
			SetFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFlag_EntityByName);
			Link(ArrayItemPin, FunctionInputPin(SetFunctionNode, TEXT("EntityHandle")));
		}
		else
		{
			SetFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFlag_TemplateByName);
			Link(ArrayItemPin, FunctionInputPin(SetFunctionNode, TEXT("TemplateData")));
		}

		Link(ProxyPin(UK2Node_SetMassFlagByName::FlagPinName()), FunctionInputPin(SetFunctionNode, TEXT("FlagName")));
			if (bIsEntityMode)
			{
				Link(ProxyPin(UK2Node_SetMassFlagByName::DeferredPinName()), FunctionInputPin(SetFunctionNode, TEXT("bDeferred")));
				if (UEdGraphPin* DelegatePin = ProxyPin(UK2Node_SetMassFlagByName::OnFinishedPinName()))
				{
					Link(DelegatePin, FunctionInputPin(SetFunctionNode, TEXT("OnFinished")));
				}
			}

		Link(SequenceNode->GetThenPinGivenIndex(0), ExecPin(SetFunctionNode));
		Link(ThenPin(SetFunctionNode), SequenceNode->GetThenPinGivenIndex(1));

		UK2Node_CallFunction* IncrementNode = HNCH_SpawnFunctionNode(UKismetMathLibrary, Add_IntInt);
		Link(LoopCounterPin, IncrementNode->FindPinChecked(TEXT("A")));
		IncrementNode->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("1");

		UK2Node_AssignmentStatement* CounterAssignNode = SpawnNode<UK2Node_AssignmentStatement>();
		CounterAssignNode->AllocateDefaultPins();
		Link(LoopCounterPin, CounterAssignNode->GetVariablePin());
		Link(IncrementNode->GetReturnValuePin(), CounterAssignNode->GetValuePin());

		Link(ProxyExecPin(), ExecPin(CounterInitNode));
		Link(ThenPin(CounterInitNode), ExecPin(BranchNode));
		Link(BranchNode->GetThenPin(), ExecPin(SequenceNode));
		Link(SequenceNode->GetThenPinGivenIndex(1), ExecPin(CounterAssignNode));
		Link(ThenPin(CounterAssignNode), ExecPin(BranchNode));
		Link(BranchNode->GetElsePin(), ProxyThenPin());

		ArrayLengthNode->PostReconstructNode();
		ArrayGetNode->PostReconstructNode();
	}
	else
	{
		// SINGLE-ENTITY/TEMPLATE PATH: Direct function call | 单实体/模板路径
		UK2Node_CallFunction* SetFunctionNode = nullptr;
		FString FunctionDataSourcePinName;

		switch (OwnerNode->CachedDataSourceType)
		{
			case EMassFragmentSourceDataType::EntityHandle:
				SetFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFlag_EntityByName);
				FunctionDataSourcePinName = TEXT("EntityHandle");
				break;
			case EMassFragmentSourceDataType::EntityTemplateData:
				SetFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFlag_TemplateByName);
				FunctionDataSourcePinName = TEXT("TemplateData");
				break;
			default:
				return;
		}

		Link(ProxyExecPin(), ExecPin(SetFunctionNode));
		Link(ThenPin(SetFunctionNode), ProxyThenPin());

		Link(ProxyPin(UK2Node_SetMassFlagByName::DataSourcePinName()), FunctionInputPin(SetFunctionNode, FunctionDataSourcePinName));
		Link(ProxyPin(UK2Node_SetMassFlagByName::FlagPinName()), FunctionInputPin(SetFunctionNode, TEXT("FlagName")));


			if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle)
			{
			Link(ProxyPin(UK2Node_SetMassFlagByName::DeferredPinName()), FunctionInputPin(SetFunctionNode, TEXT("bDeferred")));
			if (UEdGraphPin* DelegatePin = ProxyPin(UK2Node_SetMassFlagByName::OnFinishedPinName()))
			{
				Link(DelegatePin, FunctionInputPin(SetFunctionNode, TEXT("OnFinished")));
			}
			}

		if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle)
		{
			UEdGraphPin* FunctionReturnPin = NodePin(SetFunctionNode, UEdGraphSchema_K2::PN_ReturnValue.ToString());
			if (FunctionReturnPin)
			{
				Link(FunctionReturnPin, ProxyPin(UK2Node_SetMassFlagByName::ReturnValuePinName()));
			}
		}
	}
}

HNCH_EndExpandNode(UK2Node_SetMassFlagByName)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
