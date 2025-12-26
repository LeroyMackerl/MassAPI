#include "Loop/MagnusNode_ForEachArray.h"

// Engine
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "SPinTypeSelector.h"

// 编译器需要的节点类
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_TemporaryVariable.h"


//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration																		========

//———————— Node.Appearance																						————

FText UMagnusNode_ForEachArray::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (NodeTitleText.ToString() == TEXT("Default"))
	{
		FString Title;

		// 如果是反向遍历，则在开头添加"Reverse"
		if (Reverse)
		{
			Title = TEXT("Reverse For Each Array");
		}
		else
		{
			Title = TEXT("For Each Array");
		}

		// 如果支持中断，则在末尾添加"With Break"
		if (CouldBreak)
		{
			Title += TEXT(" With Break");
		}

		return FText::FromString(Title);
	}
	return NodeTitleText;
}

FText UMagnusNode_ForEachArray::GetTooltipText() const
{
	return FText::FromString(TEXT("Loops through each value in the array."));
}

FText UMagnusNode_ForEachArray::GetMenuCategory() const
{
	return FText::FromString(TEXT("Utilities|Magnus"));
}

FSlateIcon UMagnusNode_ForEachArray::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor(0.95f, 0.65f, 0.15f);
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Macro.ForEach_16x");
	return Icon;
}

FLinearColor UMagnusNode_ForEachArray::GetNodeTitleColor() const
{
	return FLinearColor(0.95f, 0.65f, 0.15f);
}


//================ Node.Properties																			========

void UMagnusNode_ForEachArray::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

//================ Pin.Management																			========

//———————— Pin.Registry																							————

const FName UMagnusNode_ForEachArray::LoopBodyPinName(TEXT("Loop Body"));
UEdGraphPin* UMagnusNode_ForEachArray::GetLoopBodyPin() const
{
	return FindPin(LoopBodyPinName);
}

const FName UMagnusNode_ForEachArray::BreakPinName(TEXT("Break"));
UEdGraphPin* UMagnusNode_ForEachArray::GetBreakPin() const
{
	return FindPin(BreakPinName);
}

const FName UMagnusNode_ForEachArray::ArrayPinName(TEXT("Array"));
UEdGraphPin* UMagnusNode_ForEachArray::GetArrayPin() const
{
	return FindPin(ArrayPinName);
}

const FName UMagnusNode_ForEachArray::ValuePinName(TEXT("Value"));
UEdGraphPin* UMagnusNode_ForEachArray::GetValuePin() const
{
	return FindPin(ValuePinName);
}

const FName UMagnusNode_ForEachArray::IndexPinName(TEXT("Index"));
UEdGraphPin* UMagnusNode_ForEachArray::GetIndexPin() const
{
	return FindPin(IndexPinName);
}

UEdGraphPin* UMagnusNode_ForEachArray::GetCompletedPin() const
{
	return FindPinChecked(UEdGraphSchema_K2::PN_Then, EGPD_Output);
}

//———————— Pin.Construction																						————

void UMagnusNode_ForEachArray::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Execute
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	// Map 
	UEdGraphNode::FCreatePinParams PinParams;
	PinParams.ContainerType = EPinContainerType::Array;
	PinParams.ValueTerminalType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;
	PinParams.ValueTerminalType.TerminalSubCategory = NAME_None;
	PinParams.ValueTerminalType.TerminalSubCategoryObject = nullptr;
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, ArrayPinName, PinParams);

	// Break
	UEdGraphPin* BreakPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, BreakPinName);
	BreakPin->PinFriendlyName = FText::FromName(BreakPinName);
	BreakPin->bHidden = !CouldBreak;
	
	// Loop body
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, LoopBodyPinName);

	// 根据遍历方向决定引脚创建顺序
	if (Reverse)
	{
		// 反向遍历时先显示索引，再显示值
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, IndexPinName);
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, ValuePinName);
	}
	else
	{
		// 正向遍历时先显示值，再显示索引
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, ValuePinName);
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, IndexPinName);
	}

	// Completed
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then)->PinFriendlyName = FText::FromName(UEdGraphSchema_K2::PN_Completed);

	RefreshPinType();
}

//———————— Pin.Notify																							————

void UMagnusNode_ForEachArray::PostReconstructNode()
{
	Super::PostReconstructNode();

	RefreshBreakPin();
	RefreshPinType();
}

void UMagnusNode_ForEachArray::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

    if (Pin == GetArrayPin() || Pin == GetValuePin())
	{
		RefreshPinType();
	}
}

//———————— Pin.Refresh																							————

void UMagnusNode_ForEachArray::RefreshBreakPin() const
{
	UEdGraphPin* BreakPin = GetBreakPin();
	if (!CouldBreak && BreakPin && BreakPin->LinkedTo.Num() > 0)
	{
		BreakPin->BreakAllPinLinks(true);
	}
}

void UMagnusNode_ForEachArray::RefreshPinType() const
{
    bool bNotifyGraphChanged = false;
    UEdGraphPin* ArrayPin = GetArrayPin();
    UEdGraphPin* ValuePin = GetValuePin();

	// 当 Array 引脚已连接时，保持其类型不变
	if (ArrayPin->LinkedTo.Num() > 0)
	{
		const UEdGraphPin* ConnectedPin = ArrayPin->LinkedTo[0];
		if (ConnectedPin && ConnectedPin->PinType.ContainerType == EPinContainerType::Array)
		{
			// 更新 Array 引脚
			ArrayPin->PinType.PinCategory = ConnectedPin->PinType.PinCategory;
			ArrayPin->PinType.PinSubCategory = ConnectedPin->PinType.PinSubCategory;
			ArrayPin->PinType.PinSubCategoryObject = ConnectedPin->PinType.PinSubCategoryObject;
			ArrayPin->PinType.ContainerType = EPinContainerType::Array;

			// 更新 Value 引脚
			ValuePin->PinType.PinCategory = ConnectedPin->PinType.PinCategory;
			ValuePin->PinType.PinSubCategory = ConnectedPin->PinType.PinSubCategory;
			ValuePin->PinType.PinSubCategoryObject = ConnectedPin->PinType.PinSubCategoryObject;

			return;
		}
	}
	
    // 无连接的情况：重置为Wildcard并断开连接
    if (ArrayPin->LinkedTo.Num() == 0 && ValuePin->LinkedTo.Num() == 0)
    {
        // 重置 Array 引脚
        ArrayPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
        ArrayPin->PinType.PinSubCategory = NAME_None;
        ArrayPin->PinType.PinSubCategoryObject = nullptr;
        ArrayPin->PinType.PinValueType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;
        ArrayPin->PinType.PinValueType.TerminalSubCategory = NAME_None;
        ArrayPin->PinType.PinValueType.TerminalSubCategoryObject = nullptr;
        ArrayPin->BreakAllPinLinks(true);

        // 重置 Value 引脚
        ValuePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
        ValuePin->PinType.PinSubCategory = NAME_None;
        ValuePin->PinType.PinSubCategoryObject = nullptr;
        ValuePin->BreakAllPinLinks(true);

        bNotifyGraphChanged = true;
    }
	
	// 只有 Array 引脚有连接：同时更新 Array 和 Value 引脚
    else if (ArrayPin->LinkedTo.Num() > 0 && ValuePin->LinkedTo.Num() == 0)
    {
    	if (ArrayPin->LinkedTo[0]->PinType.ContainerType == EPinContainerType::Array)
    	{
    		// 更新 Array 引脚
    		ArrayPin->PinType.PinCategory = ArrayPin->LinkedTo[0]->PinType.PinCategory;
    		ArrayPin->PinType.PinSubCategory = ArrayPin->LinkedTo[0]->PinType.PinSubCategory;
    		ArrayPin->PinType.PinSubCategoryObject = ArrayPin->LinkedTo[0]->PinType.PinSubCategoryObject;
    		ArrayPin->PinType.ContainerType = EPinContainerType::Array;
        
    		// 更新 Value 引脚
    		ValuePin->PinType.PinCategory = ArrayPin->LinkedTo[0]->PinType.PinCategory;
    		ValuePin->PinType.PinSubCategory = ArrayPin->LinkedTo[0]->PinType.PinSubCategory;
    		ValuePin->PinType.PinSubCategoryObject = ArrayPin->LinkedTo[0]->PinType.PinSubCategoryObject;
        
    		bNotifyGraphChanged = true;
    	}
    }

	// 只有 Value 引脚有连接：同时更新 Array 和 Value 引脚
    else if (ArrayPin->LinkedTo.Num() == 0 && ValuePin->LinkedTo.Num() > 0)
    {
    	// 更新 Array 引脚
    	ArrayPin->PinType.PinCategory = ValuePin->LinkedTo[0]->PinType.PinCategory;
    	ArrayPin->PinType.PinSubCategory = ValuePin->LinkedTo[0]->PinType.PinSubCategory;
    	ArrayPin->PinType.PinSubCategoryObject = ValuePin->LinkedTo[0]->PinType.PinSubCategoryObject;
    	ArrayPin->PinType.ContainerType = EPinContainerType::Array;
    
    	// 更新 Value 引脚
    	ValuePin->PinType.PinCategory = ValuePin->LinkedTo[0]->PinType.PinCategory;
    	ValuePin->PinType.PinSubCategory = ValuePin->LinkedTo[0]->PinType.PinSubCategory;
    	ValuePin->PinType.PinSubCategoryObject = ValuePin->LinkedTo[0]->PinType.PinSubCategoryObject;
    
    	bNotifyGraphChanged = true;
    }
	
    // 两个引脚都有连接：不做处理
    if (bNotifyGraphChanged)
    {
        GetGraph()->NotifyGraphChanged();
    }
}

//———————— Pin.Connection																						————

bool UMagnusNode_ForEachArray::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

//================ Blueprint.Integration																		========

//———————— Blueprint.Menu																							————

void UMagnusNode_ForEachArray::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	const UClass* Action = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(Action))
	{
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);

		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

//———————— Blueprint.Compile																						————

void UMagnusNode_ForEachArray::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	// 检查 Array 引脚是否连接
	UEdGraphPin* ArrayPin = GetArrayPin();
	if (ArrayPin->LinkedTo.Num() == 0)
	{
		CompilerContext.MessageLog.Error(TEXT("Please Connect Array Pin @@"), this);
		return;
	}

	// 缓存常用类引用
	static UClass* MathLibClass = nullptr;
	static UClass* ArrayLibClass = nullptr;
	if (!MathLibClass) MathLibClass = UKismetMathLibrary::StaticClass();
	if (!ArrayLibClass) ArrayLibClass = UKismetArrayLibrary::StaticClass();

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

	// 一次性预先创建所有中间节点
	struct FNodesCollection {
		UK2Node_TemporaryVariable* LoopCounterNode;
		UK2Node_AssignmentStatement* LoopCounterInitialise;
		UK2Node_IfThenElse* Branch;
		UK2Node_CallFunction* Condition;
		UK2Node_CallFunction* Length;
		UK2Node_AssignmentStatement* LoopCounterBreak;
		UK2Node_ExecutionSequence* Sequence;
		UK2Node_CallFunction* Increment;
		UK2Node_AssignmentStatement* LoopCounterAssign;
		UK2Node_CallFunction* GetValue;
		// 反向遍历可能需要的额外节点
		UK2Node_CallFunction* LengthMinus1;
	} Nodes;

	// 创建所有中间节点
	Nodes.LoopCounterNode = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(this, SourceGraph);
	Nodes.LoopCounterInitialise = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	Nodes.Branch = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
	Nodes.Condition = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	Nodes.Length = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	Nodes.LoopCounterBreak = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	Nodes.Sequence = CompilerContext.SpawnIntermediateNode<UK2Node_ExecutionSequence>(this, SourceGraph);
	Nodes.Increment = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	Nodes.LoopCounterAssign = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	Nodes.GetValue = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

	// 循环计数器
	Nodes.LoopCounterNode->VariableType.PinCategory = UEdGraphSchema_K2::PC_Int;
	Nodes.LoopCounterNode->AllocateDefaultPins();
	UEdGraphPin* LoopCounterPin = Nodes.LoopCounterNode->GetVariablePin();

	// 数组长度
	Nodes.Length->SetFromFunction(ArrayLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Length)));
	Nodes.Length->AllocateDefaultPins();
	UEdGraphPin* LengthTargetArrayPin = Nodes.Length->FindPinChecked(TEXT("TargetArray"), EGPD_Input);

	// 设置引脚类型
	FEdGraphPinType ArrayPinType = ArrayPin->PinType;
	LengthTargetArrayPin->PinType = ArrayPinType;
	LengthTargetArrayPin->PinType.PinValueType = FEdGraphTerminalType(ArrayPinType.PinValueType);

	// 根据Reverse为真或假配置不同的循环逻辑
	if (Reverse)
	{
		// 初始化循环计数器为数组长度-1
		Nodes.LengthMinus1 = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		Nodes.LengthMinus1->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt)));
		Nodes.LengthMinus1->AllocateDefaultPins();
		Nodes.LengthMinus1->FindPinChecked(TEXT("A"))->MakeLinkTo(Nodes.Length->GetReturnValuePin());
		Nodes.LengthMinus1->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("1");

		// 设置初始值
		Nodes.LoopCounterInitialise->AllocateDefaultPins();
		Nodes.LoopCounterInitialise->GetValuePin()->MakeLinkTo(Nodes.LengthMinus1->GetReturnValuePin());

		// 循环条件：判断索引是否大于等于0
		Nodes.Condition->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_IntInt)));
		Nodes.Condition->AllocateDefaultPins();
		Nodes.Condition->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("0");

		// 设置递减逻辑
		Nodes.Increment->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt)));
		Nodes.Increment->AllocateDefaultPins();
		Nodes.Increment->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("1");

		// Break：设为-1
		Nodes.LoopCounterBreak->AllocateDefaultPins();
		Nodes.LoopCounterBreak->GetValuePin()->DefaultValue = TEXT("-1");
	}
	else
	{
		// 正常循环：从0开始
		Nodes.LoopCounterInitialise->AllocateDefaultPins();
		Nodes.LoopCounterInitialise->GetValuePin()->DefaultValue = TEXT("0");

		// 循环条件：判断索引是否小于数组长度
		Nodes.Condition->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_IntInt)));
		Nodes.Condition->AllocateDefaultPins();

		// 设置递增逻辑
		Nodes.Increment->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt)));
		Nodes.Increment->AllocateDefaultPins();
		Nodes.Increment->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("1");

		// Break：设为数组长度
		Nodes.LoopCounterBreak->AllocateDefaultPins();
		Nodes.LoopCounterBreak->GetValuePin()->MakeLinkTo(Nodes.Length->GetReturnValuePin());
	}

	UEdGraphPin* LoopCounterInitialiseExecPin = Nodes.LoopCounterInitialise->GetExecPin();

	// 分支
	Nodes.Branch->AllocateDefaultPins();
	UEdGraphPin* BranchElsePin = Nodes.Branch->GetElsePin();

	UEdGraphPin* LoopConditionBPin = Reverse ?
		Nodes.Condition->FindPinChecked(TEXT("B")) :
		Nodes.Condition->FindPinChecked(TEXT("B"));

	if (!Reverse)
	{
		LoopConditionBPin->MakeLinkTo(Nodes.Length->GetReturnValuePin());
	}

	UEdGraphPin* LoopCounterBreakExecPin = Nodes.LoopCounterBreak->GetExecPin();

	// 序列
	Nodes.Sequence->AllocateDefaultPins();
	UEdGraphPin* SequenceThen0Pin = Nodes.Sequence->GetThenPinGivenIndex(0);
	UEdGraphPin* SequenceThen1Pin = Nodes.Sequence->GetThenPinGivenIndex(1);

	// 循环计数器赋值
	Nodes.LoopCounterAssign->AllocateDefaultPins();

	// 获取值
	Nodes.GetValue->SetFromFunction(ArrayLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Get)));
	Nodes.GetValue->AllocateDefaultPins();
	UEdGraphPin* GetValueTargetArrayPin = Nodes.GetValue->FindPinChecked(TEXT("TargetArray"), EGPD_Input);
	GetValueTargetArrayPin->PinType = ArrayPinType;
	GetValueTargetArrayPin->PinType.PinValueType = FEdGraphTerminalType(ArrayPinType.PinValueType);
	UEdGraphPin* ValuePin = Nodes.GetValue->FindPinChecked(TEXT("Item"));
	ValuePin->PinType = GetValuePin()->PinType;

	// 收集所有需要连接的引脚对
	struct FPinPair { UEdGraphPin* Source; UEdGraphPin* Target; };
	TArray<FPinPair> PinsToConnect;

	// 添加基本连接
	PinsToConnect.Add({ LoopCounterPin, Nodes.LoopCounterInitialise->GetVariablePin() });
	PinsToConnect.Add({ Nodes.LoopCounterInitialise->GetThenPin(), Nodes.Branch->GetExecPin() });
	PinsToConnect.Add({ Nodes.Condition->GetReturnValuePin(), Nodes.Branch->GetConditionPin() });
	PinsToConnect.Add({ Nodes.Condition->FindPinChecked(TEXT("A")), LoopCounterPin });
	PinsToConnect.Add({ Nodes.LoopCounterBreak->GetVariablePin(), LoopCounterPin });
	PinsToConnect.Add({ Nodes.Sequence->GetExecPin(), Nodes.Branch->GetThenPin() });
	PinsToConnect.Add({ Nodes.Increment->FindPinChecked(TEXT("A")), LoopCounterPin });
	PinsToConnect.Add({ Nodes.LoopCounterAssign->GetExecPin(), SequenceThen1Pin });
	PinsToConnect.Add({ Nodes.LoopCounterAssign->GetVariablePin(), LoopCounterPin });
	PinsToConnect.Add({ Nodes.LoopCounterAssign->GetValuePin(), Nodes.Increment->GetReturnValuePin() });
	PinsToConnect.Add({ Nodes.LoopCounterAssign->GetThenPin(), Nodes.Branch->GetExecPin() });
	PinsToConnect.Add({ Nodes.GetValue->FindPinChecked(TEXT("Index")), LoopCounterPin });

	// 批量处理连接
	bool bResult = true;
	TArray<FString> ErrorMessages;

	for (const FPinPair& Pair : PinsToConnect)
	{
		if (!Schema->TryCreateConnection(Pair.Source, Pair.Target))
		{
			bResult = false;
			ErrorMessages.Add(TEXT("Cannot Create Pin Connection"));
		}
	}

	// 复制引脚连接
	CompilerContext.CopyPinLinksToIntermediate(*ArrayPin, *LengthTargetArrayPin);
	CompilerContext.CopyPinLinksToIntermediate(*ArrayPin, *GetValueTargetArrayPin);

	// 移动引脚连接
	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *LoopCounterInitialiseExecPin);
	CompilerContext.MovePinLinksToIntermediate(*GetLoopBodyPin(), *SequenceThen0Pin);
	CompilerContext.MovePinLinksToIntermediate(*GetCompletedPin(), *BranchElsePin);
	CompilerContext.MovePinLinksToIntermediate(*GetBreakPin(), *LoopCounterBreakExecPin);
	CompilerContext.MovePinLinksToIntermediate(*GetValuePin(), *ValuePin);
	CompilerContext.MovePinLinksToIntermediate(*GetIndexPin(), *LoopCounterPin);

	// 一次性重构需要的节点
	TArray<UK2Node_CallFunction*> NodesToReconstruct = { Nodes.Length, Nodes.GetValue };
	if (Reverse) NodesToReconstruct.Add(Nodes.LengthMinus1);
	for (UK2Node_CallFunction* Node : NodesToReconstruct)
	{
		Node->PostReconstructNode();
	}

	// 一次性检查并报告错误
	if (!bResult)
	{
		for (const FString& Error : ErrorMessages)
		{
			CompilerContext.MessageLog.Error(*Error, this);
		}
	}

	BreakAllNodeLinks();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————