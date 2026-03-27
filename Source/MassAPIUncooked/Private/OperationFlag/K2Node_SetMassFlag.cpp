/*
* MassAPI
* Created: 2025
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#include "OperationFlag/K2Node_SetMassFlag.h"
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

namespace UK2Node_SetMassFlagHelper
{
	// DataSource类型到标题的映射
	static const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,					TEXT("SetMassFlag") },
		{ EMassFragmentSourceDataType::EntityHandle,			TEXT("SetMassFlag-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,		TEXT("SetMassFlag-Template") },
		{ EMassFragmentSourceDataType::EntityHandleArray,		TEXT("SetMassFlag-Entities") },
		{ EMassFragmentSourceDataType::EntityTemplateDataArray,	TEXT("SetMassFlag-Templates") }
	};

	// 2. Set - Icon Colors
	static const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceIconColors =
	{
		{ EMassFragmentSourceDataType::None,                    FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,            FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,      FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandleArray,       FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateDataArray, FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
	};

	// 2. Set - Title Colors
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

FText UK2Node_SetMassFlag::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(UK2Node_SetMassFlagHelper::DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_SetMassFlag::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Flag"));
}

FText UK2Node_SetMassFlag::GetTooltipText() const
{
	FString Tooltip = TEXT("Set a flag on an entity or entity template.");

	if (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray ||
		CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateDataArray)
	{
		Tooltip += TEXT(" (Array mode: processes each element in a loop.)");
	}

	return FText::FromString(Tooltip);
}

FSlateIcon UK2Node_SetMassFlag::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = UK2Node_SetMassFlagHelper::DataSourceIconColors.FindRef(CachedDataSourceType);

	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UK2Node_SetMassFlag::GetNodeTitleColor() const
{
	return UK2Node_SetMassFlagHelper::DataSourceTitleColors.FindRef(CachedDataSourceType);
}

//================ Pin.Management																				========

//———————— Pin.Construction																							————

void UK2Node_SetMassFlag::AllocateDefaultPins()
{
	// 创建执行引脚
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// 创建DataSource引脚（Wildcard，支持FEntityHandle和FEntityTemplateData）
	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsReference = true;

	// 创建Flag引脚（EEntityFlags枚举）
	UEdGraphPin* FlagPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte, StaticEnum<EEntityFlags>(), FlagPinName());

	// 创建返回值引脚（bool类型，仅EntityHandle模式有效）
	// Template模式下会在UpdateDataSourceType中移除
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());

	Super::AllocateDefaultPins();

	// 初始化DataSource类型
	UpdateDataSourceType();
}

//———————— Pin.Modification																							————

void UK2Node_SetMassFlag::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	// 恢复DataSource引脚的类型
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin && OldPin->PinName == DataSourcePinName())
		{
			if (UEdGraphPin* NewDataSourcePin = FindPin(DataSourcePinName()))
			{
				NewDataSourcePin->PinType = OldPin->PinType;
				NewDataSourcePin->PinType.bIsReference = true; // 确保引用标志被保留
			}
			break;
		}
	}

	// 更新缓存的DataSource类型
	UpdateDataSourceType();
}

void UK2Node_SetMassFlag::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
}

//———————— Pin.Connection																							————

bool UK2Node_SetMassFlag::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	// 限制DataSource引脚只能连接FEntityHandle或FEntityTemplateData
	if (MyPin && MyPin->PinName == DataSourcePinName())
	{
		// 不允许连接到Wildcard
		if (OtherPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			OutReason = TEXT("DataSource cannot connect to Wildcard. It must be either FEntityHandle or FEntityTemplateData.");
			return true;
		}

		// 检查是否为有效的结构体类型
		if (const UScriptStruct* PinStruct = Cast<UScriptStruct>(OtherPin->PinType.PinSubCategoryObject.Get()))
		{
			// 只允许这两种类型
			if (PinStruct != FEntityHandle::StaticStruct() && PinStruct != FEntityTemplateData::StaticStruct())
			{
				OutReason = TEXT("DataSource must be either FEntityHandle or FEntityTemplateData.");
				return true;
			}
		}
		else
		{
			// 不是结构体类型，拒绝连接
			OutReason = TEXT("DataSource must be either FEntityHandle or FEntityTemplateData.");
			return true;
		}
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_SetMassFlag::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	// 当DataSource引脚连接改变时，更新引脚类型和缓存
	if (Pin && Pin->PinName == DataSourcePinName())
	{
		UEdGraphPin* DataSourcePin = FindPin(DataSourcePinName());
		if (DataSourcePin)
		{
			if (DataSourcePin->LinkedTo.Num() > 0)
			{
				// 有连接，同步类型
				UEdGraphPin* LinkedPin = DataSourcePin->LinkedTo[0];
				if (LinkedPin && LinkedPin->PinType.PinSubCategoryObject.IsValid())
				{
					DataSourcePin->PinType = LinkedPin->PinType;
					DataSourcePin->PinType.bIsReference = true; // 确保引用标志被保留
				}
			}
			else
			{
				// 断开连接，恢复为Wildcard
				DataSourcePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
				DataSourcePin->PinType.PinSubCategoryObject = nullptr;
				DataSourcePin->PinType.bIsReference = true; // 确保引用标志被保留
			}

			// 更新缓存的类型
			UpdateDataSourceType();
			GetGraph()->NotifyNodeChanged(this);
		}
	}
}

//================ DataSource.Access			========

void UK2Node_SetMassFlag::UpdateDataSourceType()
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

	EMassFragmentSourceDataType OldType = CachedDataSourceType;

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

	// 根据DataSource类型管理返回值引脚
	// EntityHandle模式（单个）：有返回值
	// Template模式或Array模式：无返回值
	UEdGraphPin* ReturnPin = FindPin(ReturnValuePinName());

	if (CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateData ||
		CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateDataArray ||
		CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray)
	{
		// Template模式或Array模式：移除返回值引脚
		if (ReturnPin)
		{
			ReturnPin->BreakAllPinLinks();
			Pins.Remove(ReturnPin);
		}
	}
	else if (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle)
	{
		// EntityHandle模式（单个）：确保有返回值引脚
		if (!ReturnPin)
		{
			CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());
		}
	}
	else if (CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		// None模式：确保有返回值引脚（默认状态）
		if (!ReturnPin)
		{
			CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());
		}
	}
}

//================ Editor.PropertyChange																		========

void UK2Node_SetMassFlag::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// 目前 SetMassFlag 没有需要特殊处理的属性
}

//================ Blueprint.Integration																		========

void UK2Node_SetMassFlag::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

HNCH_StartExpandNode(UK2Node_SetMassFlag)

virtual void Compile() override
{
	// 在编译前再次更新DataSourceType，确保类型正确
	OwnerNode->UpdateDataSourceType();

	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		CompilerContext.MessageLog.Error(TEXT("DataSource must be connected to either FEntityHandle or FEntityTemplateData. @@"), OwnerNode);
		return;
	}

	// Validate array mode
	bool bIsArray = (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray ||
					 OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateDataArray);

	if (bIsArray)
	{
		UEdGraphPin* DataSourcePin = ProxyPin(UK2Node_SetMassFlag::DataSourcePinName());
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
		LengthArrayPin->PinType = ProxyPin(UK2Node_SetMassFlag::DataSourcePinName())->PinType;
		CompilerContext.CopyPinLinksToIntermediate(*ProxyPin(UK2Node_SetMassFlag::DataSourcePinName()), *LengthArrayPin);

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
		GetArrayPin->PinType = ProxyPin(UK2Node_SetMassFlag::DataSourcePinName())->PinType;
		CompilerContext.CopyPinLinksToIntermediate(*ProxyPin(UK2Node_SetMassFlag::DataSourcePinName()), *GetArrayPin);
		Link(LoopCounterPin, ArrayGetNode->FindPinChecked(TEXT("Index")));

		// Set array element output pin type (copy from array but change container to None)
		UEdGraphPin* ArrayItemPin = ArrayGetNode->FindPinChecked(TEXT("Item"));
		ArrayItemPin->PinType = ProxyPin(UK2Node_SetMassFlag::DataSourcePinName())->PinType;
		ArrayItemPin->PinType.ContainerType = EPinContainerType::None;

		// 8. Create SetFlag function call for each element
		UK2Node_CallFunction* SetFunctionNode;

		if (bIsEntityMode)
		{
			SetFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFlag_Entity);
			Link(ArrayItemPin, FunctionInputPin(SetFunctionNode, TEXT("EntityHandle")));
		}
		else
		{
			SetFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFlag_Template);
			Link(ArrayItemPin, FunctionInputPin(SetFunctionNode, TEXT("TemplateData")));
		}

		Link(ProxyPin(UK2Node_SetMassFlag::FlagPinName()), FunctionInputPin(SetFunctionNode, TEXT("FlagToSet")));

		// Connect execution flow
		Link(SequenceNode->GetThenPinGivenIndex(0), ExecPin(SetFunctionNode));
		Link(ThenPin(SetFunctionNode), SequenceNode->GetThenPinGivenIndex(1));

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
		UK2Node_CallFunction* SetFunctionNode = nullptr;
		FString FunctionDataSourcePinName;

		switch (OwnerNode->CachedDataSourceType)
		{
			case EMassFragmentSourceDataType::EntityHandle:
				SetFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFlag_Entity);
				FunctionDataSourcePinName = TEXT("EntityHandle");
				break;
			case EMassFragmentSourceDataType::EntityTemplateData:
				SetFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFlag_Template);
				FunctionDataSourcePinName = TEXT("TemplateData");
				break;
			default:
				return;
		}

		// 连接执行流
		Link(ProxyExecPin(), ExecPin(SetFunctionNode));
		Link(ThenPin(SetFunctionNode), ProxyThenPin());

		// 连接参数引脚
		Link(ProxyPin(UK2Node_SetMassFlag::DataSourcePinName()), FunctionInputPin(SetFunctionNode, FunctionDataSourcePinName));
		Link(ProxyPin(UK2Node_SetMassFlag::FlagPinName()), FunctionInputPin(SetFunctionNode, TEXT("FlagToSet")));

		// 连接返回值引脚（仅EntityHandle模式）
		if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle)
		{
			UEdGraphPin* FunctionReturnPin = NodePin(SetFunctionNode, UEdGraphSchema_K2::PN_ReturnValue.ToString());
			if (FunctionReturnPin)
			{
				Link(FunctionReturnPin, ProxyPin(UK2Node_SetMassFlag::ReturnValuePinName()));
			}
		}
	}
}

HNCH_EndExpandNode(UK2Node_SetMassFlag)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
