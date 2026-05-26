/*
* MassAPI
* Created: 2025
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#include "OperationFragment/K2Node_ForEachMatchingEntities.h"
#include "MassAPIFuncLib.h"
#include "MassAPIStructs.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "KismetCompiler.h"
#include "NodeCompiler/Magnus_HyperNodeCompilerHandler.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace UK2Node_ForEachMatchingEntitiesHelper
{
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration																			========

//———————— Node.Appearance																							————

FText UK2Node_ForEachMatchingEntities::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("ForEachMatchingEntities"));
}

FText UK2Node_ForEachMatchingEntities::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Query"));
}

FText UK2Node_ForEachMatchingEntities::GetTooltipText() const
{
	return FText::FromString(TEXT("Cursor-based iteration over all entities matching an FEntityQuery.\nFaster than the engine ForEachLoop — still, prototyping only.\nLoopBody fires per entity (Element=FEntityHandle, Index=int32).\nCompleted fires when done or query is empty.\n"
		"游标式实体遍历 | 比引擎自带ForEachLoop快，但仍推荐仅prototyping使用。\nLoopBody每实体触发一次，提供Element和Index；Completed在遍历结束或查询为空时触发。"));
}

FSlateIcon UK2Node_ForEachMatchingEntities::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor(0.9f, 0.5f, 0.2f);
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Macro.Loop_16x");
	return Icon;
}

FLinearColor UK2Node_ForEachMatchingEntities::GetNodeTitleColor() const
{
	return FLinearColor(0.9f, 0.5f, 0.2f);
}

//================ Pin.Management																				========

//———————— Pin.Construction																							————

void UK2Node_ForEachMatchingEntities::AllocateDefaultPins()
{
	// 创建输入执行引脚
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

	// 创建Query引脚（引用类型）
	UEdGraphPin* QueryPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, FEntityQuery::StaticStruct(), QueryPinName());
	QueryPin->PinType.bIsReference = true;

	// 创建输出执行引脚
	// LoopBody - 每次迭代时执行
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, LoopBodyPinName());

	// 创建输出数据引脚
	// Element - 当前实体
	UEdGraphPin* ElementPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct, FEntityHandle::StaticStruct(), ElementPinName());

	// Index - 当前索引
	UEdGraphPin* IndexPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, IndexPinName());

	// Completed - 循环完成后执行
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, CompletedPinName());

	Super::AllocateDefaultPins();
}

//================ Blueprint.Integration																		========

void UK2Node_ForEachMatchingEntities::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

//================ Compiler.Handler																				========

HNCH_StartExpandNode(UK2Node_ForEachMatchingEntities)

virtual void Compile() override
{
	// Cursor-based exec feedback loop — Begin stores result set, Advance steps one element per call | 游标驱动执行反馈循环

	// 1. Run query + store entity list, return cursor ID | 执行查询+缓存实体列表，返回游标ID
	UK2Node_CallFunction* CursorSetup = HNCH_SpawnFunctionNode(UMassAPIFuncLib, BeginEntityForEach);
	Link(ProxyExecPin(), ExecPin(CursorSetup));
	Link(ProxyPin(UK2Node_ForEachMatchingEntities::QueryPinName()), FunctionInputPin(CursorSetup, TEXT("Query")));

	// 2. Step cursor → returns bool + fills Element/Index when true | 步进游标 → 返回bool，为true时填充Element/Index
	UK2Node_CallFunction* CursorStep = HNCH_SpawnFunctionNode(UMassAPIFuncLib, AdvanceEntityForEach);
	Link(ThenPin(CursorSetup), ExecPin(CursorStep));
	Link(FunctionReturnPin(CursorSetup), FunctionInputPin(CursorStep, TEXT("IterId")));

	// 3. Output data: Element + Index → proxy pins — valid only when CursorStep returns true | 输出数据：当CursorStep返回true时有效
	Link(NodePin(CursorStep, TEXT("OutElement")), ProxyPin(UK2Node_ForEachMatchingEntities::ElementPinName()));
	Link(NodePin(CursorStep, TEXT("OutIndex")), ProxyPin(UK2Node_ForEachMatchingEntities::IndexPinName()));

	// 4. Gate: has more entities? | 门控：还有更多实体？
	UK2Node_IfThenElse* MoreCheck = SpawnBranchNode();
	Link(ThenPin(CursorStep), ExecPin(MoreCheck));
	Link(FunctionReturnPin(CursorStep), BranchConditionPin(MoreCheck));

	// 5. Loop body dispatch + feedback edge | 循环体调度+反馈边
	UK2Node_ExecutionSequence* BodyThenLoopback = SpawnSequenceNode(2);
	Link(BranchTruePin(MoreCheck), ExecPin(BodyThenLoopback));

	Link(SequenceOutPin(BodyThenLoopback, 0), ProxyPin(UK2Node_ForEachMatchingEntities::LoopBodyPinName()));
	Link(SequenceOutPin(BodyThenLoopback, 1), ExecPin(CursorStep));  // feedback edge — re-enters CursorStep | 反馈边 — 重新进入CursorStep

	// 6. No more → Completed | 迭代结束 → 完成
	Link(BranchFalsePin(MoreCheck), ProxyPin(UK2Node_ForEachMatchingEntities::CompletedPinName()));
}

HNCH_EndExpandNode(UK2Node_ForEachMatchingEntities)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
