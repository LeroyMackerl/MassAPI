// Leroy Works & Ember, All Rights Reserved.

#include "OperationFragment/K2Node_ForEachMatchingEntities.h"
#include "MassAPIFuncLib.h"
#include "MassAPISubsystem.h"
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
	return FText::FromString(TEXT("Iterate over all entities matching the specified query. Use it for prototyping only, because for each loop in BP is slow"));
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
	// 策略：使用临时 bool 变量实现 DoOnce 逻辑，确保委托只绑定一次
	//
	// 执行流程：
	//   Execute → Sequence
	//           → [0] Branch(bInitialized)
	//                 → False: CreateHelper → BindDelegate → SetInitialized(true)
	//                 → True: (跳过)
	//           → [1] GetHelper → ExecuteForEach → Completed

	// 1. 创建 Sequence 节点（2个输出：初始化分支 + 主流程）
	UK2Node_ExecutionSequence* InitSequence = SpawnSequenceNode(2);
	Link(ProxyExecPin(), ExecPin(InitSequence));

	// 2. 创建临时 bool 变量（用于追踪是否已初始化）
	UK2Node_TemporaryVariable* InitializedVar = SpawnTempVarNode(
		UEdGraphSchema_K2::PC_Boolean
	);

	// 3. 创建 Branch 节点检查是否已初始化
	UK2Node_IfThenElse* InitCheckBranch = SpawnBranchNode();
	Link(SequenceOutPin(InitSequence, 0), ExecPin(InitCheckBranch));
	Link(TempValuePin(InitializedVar), BranchConditionPin(InitCheckBranch));

	// 4. Branch.False 分支：首次执行，创建并绑定 Helper
	UK2Node_CallFunction* CreateHelperNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, ForEachMatchingEntities);
	Link(BranchFalsePin(InitCheckBranch), ExecPin(CreateHelperNode));

	UEdGraphPin* HelperObjectPin = FunctionReturnPin(CreateHelperNode);

	UEdGraphPin* LastThenPin = ThenPin(CreateHelperNode);
	UEdGraphPin* EventThenPin = nullptr;

	bool bSuccess = CreateAsyncDelegateBinding(
		UMassAPISubsystem::StaticClass(),
		GET_MEMBER_NAME_CHECKED(UMassAPISubsystem, OnEntityIterate),
		HelperObjectPin,
		LastThenPin,
		EventThenPin
	);

	if (!bSuccess || !EventThenPin)
	{
		CompilerContext.MessageLog.Error(TEXT("Failed to create delegate binding for ForEachMatchingEntities. @@"), OwnerNode);
		return;
	}

	// 5. 连接事件的输出引脚到 LoopBody
	Link(EventThenPin, ProxyPin(UK2Node_ForEachMatchingEntities::LoopBodyPinName()));

	// 6. 连接委托参数到输出数据引脚
	UEdGraphNode* CustomEventNode = EventThenPin->GetOwningNode();
	if (CustomEventNode)
	{
		if (UEdGraphPin* ElementEventPin = CustomEventNode->FindPin(TEXT("Element")))
			Link(ElementEventPin, ProxyPin(UK2Node_ForEachMatchingEntities::ElementPinName()));

		if (UEdGraphPin* IndexEventPin = CustomEventNode->FindPin(TEXT("Index")))
			Link(IndexEventPin, ProxyPin(UK2Node_ForEachMatchingEntities::IndexPinName()));
	}

	// 7. 绑定完成后，设置 bInitialized = true
	UK2Node_AssignmentStatement* SetInitializedNode = SpawnAssignNode(TempValuePin(InitializedVar), TEXT("true"));
	Link(LastThenPin, ExecPin(SetInitializedNode));

	// 8. Sequence[1]: 主流程 - 每次都执行
	UK2Node_CallFunction* GetHelperNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, ForEachMatchingEntities);
	Link(SequenceOutPin(InitSequence, 1), ExecPin(GetHelperNode));

	UEdGraphPin* CachedHelperPin = FunctionReturnPin(GetHelperNode);

	// 9. 调用 ExecuteForEach 来执行遍历
	UK2Node_CallFunction* ExecuteForEachNode = HNCH_SpawnFunctionNode(UMassAPISubsystem, ExecuteForEach);

	Link(ThenPin(GetHelperNode), ExecPin(ExecuteForEachNode));
	Link(CachedHelperPin, FunctionTargetPin(ExecuteForEachNode));
	Link(ProxyPin(UK2Node_ForEachMatchingEntities::QueryPinName()), FunctionInputPin(ExecuteForEachNode, TEXT("Query")));

	// 10. ExecuteForEach 完成后连接到 Completed 引脚
	Link(ThenPin(ExecuteForEachNode), ProxyPin(UK2Node_ForEachMatchingEntities::CompletedPinName()));
}

HNCH_EndExpandNode(UK2Node_ForEachMatchingEntities)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
