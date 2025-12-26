// Leroy Works & Ember, All Rights Reserved.

#pragma once

#include "KismetCompiler.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h"

// 节点类
#include "K2Node_Knot.h"
#include "K2Node_TemporaryVariable.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_Select.h"
#include "K2Node_MakeMap.h"
#include "K2Node_MakeArray.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_CreateDelegate.h"
#include "K2Node_BaseAsyncTask.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"

#include "K2Node_CallFunction.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// 创建函数节点
#define HNCH_SpawnFunctionNode(Class, Function) \
[this]() { \
UK2Node_CallFunction* Node = SpawnNode<UK2Node_CallFunction>(); \
Node->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(Class, Function), Class::StaticClass()); \
Node->AllocateDefaultPins(); \
return Node; \
}()

// 定义编译器Handler类的开始和结束宏
#define HNCH_StartExpandNode(NodeClass) \
class F##NodeClass##CompilerHandler : public FHyperNodeCompilerHandler \
{ \
public: \
	F##NodeClass##CompilerHandler(FKismetCompilerContext& InCompilerContext, UEdGraph* InSourceGraph, NodeClass* InNode) \
		: FHyperNodeCompilerHandler(InCompilerContext, InSourceGraph, InNode) \
		, OwnerNode(InNode) \
	{ \
	} \
	NodeClass* OwnerNode;

#define HNCH_EndExpandNode(NodeClass) \
}; \
void NodeClass::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) \
{ \
	Super::ExpandNode(CompilerContext, SourceGraph); \
	F##NodeClass##CompilerHandler Handler(CompilerContext, SourceGraph, this); \
	Handler.Execute(); \
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class MAGNUSEXTENSION_API FHyperNodeCompilerHandler
{

public:

	//================ Compiler.Utilities																		========

	//———————— Compiler.Construction																				————

	FHyperNodeCompilerHandler(FKismetCompilerContext& InCompilerContext, UEdGraph* InSourceGraph, UEdGraphNode* InNode);
	virtual ~FHyperNodeCompilerHandler() {}

	//———————— Compiler.Access																						————

	void Execute();
	void Initialize();
	virtual void Compile() {}

	//================ Pin.Access																				========

	//———————— Pin.Sync																								————

	void AutoSyncPinType(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

	//———————— Pin.Link																								————

	void Link(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

	//———————— Pin.DefaultValue																						————

	void PinDefaultValue(UEdGraphPin* Pin, const FString& Value);
	void PinDefaultValueInNode(UK2Node* InNode, const FString PinName, const FString& Value);
	void PinDefaultValueInAssignNode(UK2Node_AssignmentStatement* AssignNode, const FString& DefaultValue);
	void PinDefaultValueInSelectNode(UK2Node_Select* SelectNode, const int32 OptionIndex, const FString& Value);

	//================ Node.Spawner																				========

	// 创建中间节点的辅助方法
	template<class NodeClass> NodeClass* SpawnNode()
	{ return CompilerContext.SpawnIntermediateNode<NodeClass>(HandlingNode, SourceGraph); }

	UK2Node_Knot* SpawnKnotNode();
	UK2Node_TemporaryVariable* SpawnTempVarNode(
		const FName PinCategory = UEdGraphSchema_K2::PC_Wildcard,
		const FName PinSubCategory = NAME_None,
		UObject* PinSubCategoryObject = nullptr,
		const FSimpleMemberReference& PinSubCategoryMemberReference = FSimpleMemberReference(),
		const FEdGraphTerminalType& PinValueType = FEdGraphTerminalType(),
		EPinContainerType ContainerType = EPinContainerType::None);
	UK2Node_TemporaryVariable* SpawnCopyTempVarNode(const UEdGraphPin* SourcePin);
	UK2Node_AssignmentStatement* SpawnAssignNode(UEdGraphPin* RefPin, const FString& DefaultValue = TEXT(""));
	UK2Node_IfThenElse* SpawnBranchNode(const bool bCondition = true);
	UK2Node_ExecutionSequence* SpawnSequenceNode(int32 ThenPinAmount = 2);
	UK2Node_SwitchEnum* SpawnSwitchEnumNode(UEnum* SwitchEnum);
	UK2Node_Select* SpawnSelectNode(UEdGraphPin* TargetPin, UEdGraphPin* IndexPin, int32 PinAmount = 2);
	UK2Node_MakeMap* SpawnMakeMapNode(UEdGraphPin* TargetPin, int32 PairCount);
	UK2Node_MakeArray* SpawnMakeArrayNode(UEdGraphPin* TargetPin, int32 ElementCount);
	UK2Node_CallFunction* SpawnEqualNameNode();
	UK2Node_CallFunction* SpawnMakeNameNode(const FString& NameValue);

	//================ Node.Delegate																			========

	bool CreateAsyncDelegateBinding(UClass* DelegateOwnerClass, const FName& DelegateName,UEdGraphPin* TargetObjectPin, UEdGraphPin*& InOutLastThenPin, UEdGraphPin*& OutEventThenPin);
	bool CreateAsyncSingleDelegateBinding(UClass* DelegateOwnerClass, const FName& DelegateName, UEdGraphPin* TargetObjectPin, UEdGraphPin*& InOutLastThenPin, UEdGraphPin*& OutEventThenPin);
	void CreateEventFilter(UEdGraphPin* EventThenPin, const FString& TargetName, UEdGraphPin* OutputPin);

	//================ Node.Access																				========

	//———————— Node.Common																							————

	UEdGraphPin* ProxyPin(const FName& PinName) const;
	UEdGraphPin* ProxyExecPin() const;
	UEdGraphPin* ProxyThenPin() const;
	UEdGraphPin* NodePin(UEdGraphNode* Node, const FString& PinName);
	UEdGraphPin* ExecPin(UEdGraphNode* Node);
	UEdGraphPin* ThenPin(UEdGraphNode* Node);

	//———————— Node.Variable																						————

	UEdGraphPin* TempValuePin(UK2Node_TemporaryVariable* TempVar);

	//———————— Node.Assignment																						————

	UEdGraphPin* AssignRefPin(UK2Node_AssignmentStatement* AssignNode);
	UEdGraphPin* AssignValuePin(UK2Node_AssignmentStatement* AssignNode);

	//———————— Node.Branch																							————

	UEdGraphPin* BranchConditionPin(UK2Node_IfThenElse* BranchNode);
	UEdGraphPin* BranchTruePin(UK2Node_IfThenElse* BranchNode);
	UEdGraphPin* BranchFalsePin(UK2Node_IfThenElse* BranchNode);

	//———————— Node.Sequence																						————

	UEdGraphPin* SequenceOutPin(UK2Node_ExecutionSequence* SequenceNode, const int32 Index);

	//———————— Node.Switch																							————

	UEdGraphPin* SwitchEnumSelectionPin(UK2Node_SwitchEnum* SwitchNode) const;
	UEdGraphPin* SwitchEnumOutPin(UK2Node_SwitchEnum* SwitchNode, const FString& EnumValue);

	//———————— Node.Select																							————

	UEdGraphPin* SelectOptionPin(UK2Node_Select* SelectNode, int32 OptionIndex);
	UEdGraphPin* SelectResultPin(UK2Node_Select* SelectNode);

	//———————— Node.Map																								————

	UEdGraphPin* MakeMapKeyPin(UK2Node_MakeMap* MakeMapNode, int32 Index);
	UEdGraphPin* MakeMapValuePin(UK2Node_MakeMap* MakeMapNode, int32 Index);
	UEdGraphPin* MakeMapResultPin(UK2Node_MakeMap* MakeMapNode);

	//———————— Node.Array																							————

	UEdGraphPin* MakeArrayElementPin(UK2Node_MakeArray* MakeArrayNode, int32 Index);
	UEdGraphPin* MakeArrayReturnPin(UK2Node_MakeArray* MakeArrayNode);

	//———————— Node.CallFunction																					————

	UEdGraphPin* FunctionTargetPin(UK2Node_CallFunction* FunctionNode);
	UEdGraphPin* FunctionInputPin(UK2Node_CallFunction* FunctionNode, const FString& PinName);
	UEdGraphPin* FunctionReturnPin(UK2Node_CallFunction* FunctionNode, const FString& PinName = TEXT(""));

	//———————— Properties.Default																					————

	FKismetCompilerContext& CompilerContext;
	UEdGraph* SourceGraph;
	UEdGraphNode* HandlingNode;
	const UEdGraphSchema_K2* Schema;

private:

	//———————— Properties.PinTracking																				————

	// 引脚使用状态
	enum class EPinUsageState
	{
		Unused,      // 未使用 - 可以使用 MovePinLinksToIntermediate
		Moved,       // 已移动 - 后续只能使用 CopyPinLinksToIntermediate
	};

	// 追踪原始引脚的使用状态
	TMap<FName, EPinUsageState> OriginalPinUsageState;

	//———————— Methods.LinkHelpers																					————

	void HandleOriginalToIntermediate(UEdGraphPin* OriginalPin, UEdGraphPin* IntermediatePin);
	void HandleIntermediateToOriginal(UEdGraphPin* IntermediatePin, UEdGraphPin* OriginalPin);
	void HandleDirectConnection(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);
	void SyncPinTypeIfNeeded(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
