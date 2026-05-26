/*
* MassAPI
* Created: 2026
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#include "OperationFragment/K2Node_ForEachEntityHandle.h"
#include "MassAPIFuncLib.h"
#include "MassAPIStructs.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "KismetCompiler.h"
#include "NodeCompiler/Magnus_HyperNodeCompilerHandler.h"

//================ Node.Configuration ========

FText UK2Node_ForEachEntityHandle::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("ForEachEntityHandle"));
}

FText UK2Node_ForEachEntityHandle::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Query"));
}

FText UK2Node_ForEachEntityHandle::GetTooltipText() const
{
	return FText::FromString(TEXT("Cursor-based iteration over a TArray<FEntityHandle>.\nFaster than the engine ForEachLoop — still, prototyping only.\nLoopBody fires per element (Element=FEntityHandle, Index=int32).\nCompleted fires when done or array is empty. Supports nesting.\n"
		"游标式数组遍历 | 比引擎自带ForEachLoop快，但仍推荐仅prototyping使用。\nLoopBody每元素触发一次，提供Element和Index；Completed在遍历结束或数组为空时触发。支持嵌套。"));
}

FSlateIcon UK2Node_ForEachEntityHandle::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor(0.9f, 0.5f, 0.2f);
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Macro.Loop_16x");
	return Icon;
}

FLinearColor UK2Node_ForEachEntityHandle::GetNodeTitleColor() const
{
	return FLinearColor(0.9f, 0.5f, 0.2f);
}

//================ Pin.Management ========

void UK2Node_ForEachEntityHandle::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

	// Array input pin: TArray<FEntityHandle> | 数组输入
	UEdGraphPin* ArrayPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, FEntityHandle::StaticStruct(), ArrayPinName());
	ArrayPin->PinType.ContainerType = EPinContainerType::Array;

	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, LoopBodyPinName());

	UEdGraphPin* ElementPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct, FEntityHandle::StaticStruct(), ElementPinName());

	UEdGraphPin* IndexPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, IndexPinName());

	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, CompletedPinName());

	Super::AllocateDefaultPins();
}

//================ Blueprint.Integration ========

void UK2Node_ForEachEntityHandle::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

//================ Compiler.Handler ========

HNCH_StartExpandNode(UK2Node_ForEachEntityHandle)

virtual void Compile() override
{
	// Cursor-based exec feedback loop — Begin copies array, Advance steps one element per call | 游标驱动执行反馈循环

	// 1. Cache array copy + reset index, return cursor ID | 缓存数组副本+重置索引，返回游标ID
	UK2Node_CallFunction* CursorSetup = HNCH_SpawnFunctionNode(UMassAPIFuncLib, BeginEntityHandleArrayForEach);
	Link(ProxyExecPin(), ExecPin(CursorSetup));
	Link(ProxyPin(UK2Node_ForEachEntityHandle::ArrayPinName()), FunctionInputPin(CursorSetup, TEXT("Array")));

	// 2. Step cursor → returns bool + fills Element/Index when true | 步进游标 → 返回bool，为true时填充Element/Index
	UK2Node_CallFunction* CursorStep = HNCH_SpawnFunctionNode(UMassAPIFuncLib, AdvanceEntityHandleArrayForEach);
	Link(ThenPin(CursorSetup), ExecPin(CursorStep));
	Link(FunctionReturnPin(CursorSetup), FunctionInputPin(CursorStep, TEXT("IterId")));

	// 3. Output data: Element + Index → proxy pins | 输出数据：元素+索引传至代理引脚
	Link(NodePin(CursorStep, TEXT("OutElement")), ProxyPin(UK2Node_ForEachEntityHandle::ElementPinName()));
	Link(NodePin(CursorStep, TEXT("OutIndex")), ProxyPin(UK2Node_ForEachEntityHandle::IndexPinName()));

	// 4. Gate: has more entities? | 门控：还有更多实体？
	UK2Node_IfThenElse* MoreCheck = SpawnBranchNode();
	Link(ThenPin(CursorStep), ExecPin(MoreCheck));
	Link(FunctionReturnPin(CursorStep), BranchConditionPin(MoreCheck));

	// 5. Loop body dispatch + feedback edge | 循环体调度+反馈边
	UK2Node_ExecutionSequence* BodyThenLoopback = SpawnSequenceNode(2);
	Link(BranchTruePin(MoreCheck), ExecPin(BodyThenLoopback));

	Link(SequenceOutPin(BodyThenLoopback, 0), ProxyPin(UK2Node_ForEachEntityHandle::LoopBodyPinName()));
	Link(SequenceOutPin(BodyThenLoopback, 1), ExecPin(CursorStep));  // feedback edge | 反馈边

	// 6. No more → Completed | 迭代结束 → 完成
	Link(BranchFalsePin(MoreCheck), ProxyPin(UK2Node_ForEachEntityHandle::CompletedPinName()));
}

HNCH_EndExpandNode(UK2Node_ForEachEntityHandle)
