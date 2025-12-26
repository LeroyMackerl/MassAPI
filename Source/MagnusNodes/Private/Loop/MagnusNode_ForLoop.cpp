#include "Loop/MagnusNode_ForLoop.h"

// Engine
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "Kismet/KismetMathLibrary.h"

// Project
#include "NodeCompiler/Magnus_HyperNodeCompilerHandler.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration																		========

//———————— Node.Appearance																						————

FText UMagnusNode_ForLoop::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (NodeTitleText.ToString() == TEXT("Default"))
	{
		FString Title = TEXT("For Loop");

		// 如果支持中断，则在末尾添加"With Break"
		if (CouldBreak)
		{
			Title += TEXT(" With Break");
		}

		return FText::FromString(Title);
	}

	return NodeTitleText;
}

FText UMagnusNode_ForLoop::GetTooltipText() const
{
	return FText::FromString(TEXT("Executes the loop body for each index in the range."));
}

FText UMagnusNode_ForLoop::GetMenuCategory() const
{
	return FText::FromString(TEXT("Utilities|Magnus"));
}

FSlateIcon UMagnusNode_ForLoop::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor(0.5f, 0.7f, 0.95f);
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Macro.Loop_16x");
	return Icon;
}

FLinearColor UMagnusNode_ForLoop::GetNodeTitleColor() const
{
	return FLinearColor(0.5f, 0.7f, 0.95f);
}

//================ Node.Properties																			========

void UMagnusNode_ForLoop::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

//================ Pin.Management																			========

//———————— Pin.Registry																							————

const FName UMagnusNode_ForLoop::LoopBodyPinName(TEXT("Loop Body"));
UEdGraphPin* UMagnusNode_ForLoop::GetLoopBodyPin() const
{
	return FindPin(LoopBodyPinName);
}

const FName UMagnusNode_ForLoop::BreakPinName(TEXT("Break"));
UEdGraphPin* UMagnusNode_ForLoop::GetBreakPin() const
{
	return FindPin(BreakPinName);
}

const FName UMagnusNode_ForLoop::FirstIndexName(TEXT("FirstIndex"));
UEdGraphPin* UMagnusNode_ForLoop::GetFirstIndexPin() const
{
	return FindPin(FirstIndexName);
}

const FName UMagnusNode_ForLoop::LastIndexName(TEXT("LastIndex"));
UEdGraphPin* UMagnusNode_ForLoop::GetLastIndexPin() const
{
	return FindPin(LastIndexName);
}

const FName UMagnusNode_ForLoop::IndexPinName(TEXT("Index"));
UEdGraphPin* UMagnusNode_ForLoop::GetIndexPin() const
{
	return FindPin(IndexPinName);
}

UEdGraphPin* UMagnusNode_ForLoop::GetCompletedPin() const
{
	return FindPinChecked(UEdGraphSchema_K2::PN_Then, EGPD_Output);
}

//———————— Pin.Construction																						————

void UMagnusNode_ForLoop::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Execute
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

	// First Index
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Int, FirstIndexName);

	// Last Index
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Int, LastIndexName);

	// Break
	UEdGraphPin* BreakPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, BreakPinName);
	BreakPin->PinFriendlyName = FText::FromName(BreakPinName);
	BreakPin->bHidden = !CouldBreak;
	
	// Loop body
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, LoopBodyPinName);

	// Index
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, IndexPinName);

	// Completed
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then)->PinFriendlyName = FText::FromName(UEdGraphSchema_K2::PN_Completed);
}

//———————— Pin.Notify																							————

void UMagnusNode_ForLoop::PostReconstructNode()
{
	Super::PostReconstructNode();
}

void UMagnusNode_ForLoop::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);
	GetGraph()->NotifyGraphChanged();
}

//———————— Pin.Connection																						————

bool UMagnusNode_ForLoop::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

//================ Blueprint.Integration																		========

//———————— Blueprint.Menu																							————

void UMagnusNode_ForLoop::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

HNCH_StartExpandNode(UMagnusNode_ForLoop)

	virtual void Compile() override
	{
		static UClass* MathLibClass = UKismetMathLibrary::StaticClass();

		// 1. 创建临时变量
		UK2Node_TemporaryVariable* LoopCounterNode = SpawnTempVarNode(UEdGraphSchema_K2::PC_Int);
		UEdGraphPin* LoopCounterPin = TempValuePin(LoopCounterNode);

		UK2Node_TemporaryVariable* BreakFlagNode = SpawnTempVarNode(UEdGraphSchema_K2::PC_Boolean);
		UEdGraphPin* BreakFlagPin = TempValuePin(BreakFlagNode);

		UK2Node_TemporaryVariable* LastIndexVarNode = SpawnTempVarNode(UEdGraphSchema_K2::PC_Int);
		UEdGraphPin* LastIndexVarPin = TempValuePin(LastIndexVarNode);

		UK2Node_TemporaryVariable* IncrementValueNode = SpawnTempVarNode(UEdGraphSchema_K2::PC_Int);
		UEdGraphPin* IncrementValuePin = TempValuePin(IncrementValueNode);

		// 2. 初始化逻辑
		// BreakFlag = false
		UK2Node_AssignmentStatement* InitBreakFlagNode = SpawnAssignNode(BreakFlagPin, "false");
		Link(ProxyExecPin(), ExecPin(InitBreakFlagNode));

		// 缓存 LastIndex
		UK2Node_AssignmentStatement* LastIndexAssignNode = SpawnAssignNode(LastIndexVarPin);
		Link(ThenPin(InitBreakFlagNode), ExecPin(LastIndexAssignNode));
		Link(ProxyPin(OwnerNode->LastIndexName), AssignValuePin(LastIndexAssignNode));

		// 确定遍历方向 (First > Last ?)
		UK2Node_CallFunction* CompareNode = SpawnNode<UK2Node_CallFunction>();
		CompareNode->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_IntInt)));
		CompareNode->AllocateDefaultPins();
		Link(ProxyPin(OwnerNode->FirstIndexName), FunctionInputPin(CompareNode, "A"));
		Link(LastIndexVarPin, FunctionInputPin(CompareNode, "B"));

		UK2Node_IfThenElse* DirBranchNode = SpawnBranchNode();
		Link(ThenPin(LastIndexAssignNode), ExecPin(DirBranchNode));
		Link(FunctionReturnPin(CompareNode), BranchConditionPin(DirBranchNode));

		// 设置步长 (Increment Value)
		UK2Node_AssignmentStatement* SetDecNode = SpawnAssignNode(IncrementValuePin, "-1");
		UK2Node_AssignmentStatement* SetIncNode = SpawnAssignNode(IncrementValuePin, "1");

		Link(BranchTruePin(DirBranchNode), ExecPin(SetDecNode));
		Link(BranchFalsePin(DirBranchNode), ExecPin(SetIncNode));

		// 初始化 Loop Counter = FirstIndex
		UK2Node_AssignmentStatement* InitLoopCounterNode = SpawnAssignNode(LoopCounterPin);
        Link(ProxyPin(OwnerNode->FirstIndexName), AssignValuePin(InitLoopCounterNode));
		
		// 连接两条路径到 LoopCounter 初始化
		Link(ThenPin(SetDecNode), ExecPin(InitLoopCounterNode));
		Link(ThenPin(SetIncNode), ExecPin(InitLoopCounterNode));

		// 3. 循环条件逻辑
		// StopValue = LastIndex + IncrementValue
		UK2Node_CallFunction* StopValueCalcNode = SpawnNode<UK2Node_CallFunction>();
		StopValueCalcNode->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt)));
		StopValueCalcNode->AllocateDefaultPins();
		Link(LastIndexVarPin, FunctionInputPin(StopValueCalcNode, "A"));
		Link(IncrementValuePin, FunctionInputPin(StopValueCalcNode, "B"));

		// LoopCounter != StopValue
		UK2Node_CallFunction* ConditionEqualNode = SpawnNode<UK2Node_CallFunction>();
		ConditionEqualNode->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_IntInt)));
		ConditionEqualNode->AllocateDefaultPins();
		Link(LoopCounterPin, FunctionInputPin(ConditionEqualNode, "A"));
		Link(FunctionReturnPin(StopValueCalcNode), FunctionInputPin(ConditionEqualNode, "B"));

		// Not(BreakFlag)
		UK2Node_CallFunction* NotBreakNode = SpawnNode<UK2Node_CallFunction>();
		NotBreakNode->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Not_PreBool)));
		NotBreakNode->AllocateDefaultPins();
		Link(BreakFlagPin, FunctionInputPin(NotBreakNode, "A"));

		// Combined Condition: (Count != Stop) AND (!Break)
		UK2Node_CallFunction* CombinedConditionNode = SpawnNode<UK2Node_CallFunction>();
		CombinedConditionNode->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND)));
		CombinedConditionNode->AllocateDefaultPins();
		Link(FunctionReturnPin(ConditionEqualNode), FunctionInputPin(CombinedConditionNode, "A"));
		Link(FunctionReturnPin(NotBreakNode), FunctionInputPin(CombinedConditionNode, "B"));

		// 4. 循环结构
		UK2Node_IfThenElse* LoopBranchNode = SpawnBranchNode();
		Link(ThenPin(InitLoopCounterNode), ExecPin(LoopBranchNode));
		Link(FunctionReturnPin(CombinedConditionNode), BranchConditionPin(LoopBranchNode));

		UK2Node_ExecutionSequence* SequenceNode = SpawnSequenceNode();
		Link(BranchTruePin(LoopBranchNode), ExecPin(SequenceNode));

		// 5. 循环体输出
		Link(SequenceOutPin(SequenceNode, 0), ProxyPin(OwnerNode->LoopBodyPinName));
		Link(LoopCounterPin, ProxyPin(OwnerNode->IndexPinName));

		// 6. 步进逻辑
		UK2Node_CallFunction* IncrementNode = SpawnNode<UK2Node_CallFunction>();
		IncrementNode->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt)));
		IncrementNode->AllocateDefaultPins();
		Link(LoopCounterPin, FunctionInputPin(IncrementNode, "A"));
		Link(IncrementValuePin, FunctionInputPin(IncrementNode, "B"));

		UK2Node_AssignmentStatement* UpdateLoopCounterNode = SpawnAssignNode(LoopCounterPin);
		Link(SequenceOutPin(SequenceNode, 1), ExecPin(UpdateLoopCounterNode));
		Link(FunctionReturnPin(IncrementNode), AssignValuePin(UpdateLoopCounterNode));
		Link(ThenPin(UpdateLoopCounterNode), ExecPin(LoopBranchNode)); // Loop Back

		// 7. Completed 和 Break 处理
		// 创建合并节点用于处理 Loop False 和 Break 之后的流程
		UK2Node_ExecutionSequence* CompletedMergerNode = SpawnSequenceNode();
		Link(BranchFalsePin(LoopBranchNode), ExecPin(CompletedMergerNode));
		Link(SequenceOutPin(CompletedMergerNode, 0), ProxyPin(UEdGraphSchema_K2::PN_Completed));

		// Break 逻辑
		UK2Node_AssignmentStatement* SetBreakFlagNode = SpawnAssignNode(BreakFlagPin, "true");
		Link(ProxyPin(OwnerNode->BreakPinName), ExecPin(SetBreakFlagNode));
		Link(ThenPin(SetBreakFlagNode), ExecPin(CompletedMergerNode));
	}

HNCH_EndExpandNode(UMagnusNode_ForLoop)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————