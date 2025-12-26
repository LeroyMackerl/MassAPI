// Leroy Works & Ember, All Rights Reserved.

#include "NodeCompiler/Magnus_HyperNodeCompilerHandler.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Compiler.Utilities																			========

//———————— Compiler.Construction																			        ————

FHyperNodeCompilerHandler::FHyperNodeCompilerHandler(FKismetCompilerContext& InCompilerContext, UEdGraph* InSourceGraph, UEdGraphNode* InNode)
	: CompilerContext(InCompilerContext)
	, SourceGraph(InSourceGraph)
	, HandlingNode(InNode)
{
	Schema = Cast<UEdGraphSchema_K2>(SourceGraph->GetSchema());
}

//———————— Compiler.Access																					        ————

void FHyperNodeCompilerHandler::Execute()
{
	Initialize();
	Compile();

	// 清理原始节点：断开所有连接
	// 这确保编译器不会尝试处理原始节点
	HandlingNode->BreakAllNodeLinks();
}

void FHyperNodeCompilerHandler::Initialize()
{
	// 初始化引脚使用状态追踪
	// 不再创建 Knot 节点，直接追踪原始引脚的使用状态
	for (UEdGraphPin* Pin : HandlingNode->Pins)
	{
		if (Pin)
		{
			OriginalPinUsageState.Add(Pin->PinName, EPinUsageState::Unused);
		}
	}
}

//================ Pin.Access																					========

//———————— Pin.Sync																							        ————

void FHyperNodeCompilerHandler::AutoSyncPinType(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
	// 检查是否需要进行类型同步
	auto ShouldSyncPinType = [](UEdGraphPin* Pin) -> bool
	{
		if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
			return Cast<UK2Node_CallFunction>(Pin->GetOwningNode()) != nullptr;
		
		return false;
	};

	auto IsWildcard = [](UEdGraphPin* Pin) -> bool
	{
		return Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard;
	};

	// 只在一端为CallFunction的Wildcard且另一端为非Wildcard时同步
	if (ShouldSyncPinType(TargetPin) && !IsWildcard(SourcePin))
		TargetPin->PinType = SourcePin->PinType;
	else if (ShouldSyncPinType(SourcePin) && !IsWildcard(TargetPin))
		SourcePin->PinType = TargetPin->PinType;
	
}

//———————— Pin.Link																							        ————

void FHyperNodeCompilerHandler::Link(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
	if (!SourcePin || !TargetPin)
		return;

	bool bSourceIsOriginal = (SourcePin->GetOwningNode() == HandlingNode);
	bool bTargetIsOriginal = (TargetPin->GetOwningNode() == HandlingNode);

	if (bSourceIsOriginal && !bTargetIsOriginal)
	{
		// 情况1: 原始节点 → Intermediate节点
		HandleOriginalToIntermediate(SourcePin, TargetPin);
	}
	else if (!bSourceIsOriginal && bTargetIsOriginal)
	{
		// 情况2: Intermediate节点 → 原始节点（反向）
		HandleIntermediateToOriginal(SourcePin, TargetPin);
	}
	else
	{
		// 情况3: Intermediate节点 ↔ Intermediate节点
		HandleDirectConnection(SourcePin, TargetPin);
	}
}

//———————— Pin.DefaultValue																					        ————

void FHyperNodeCompilerHandler::PinDefaultValue(UEdGraphPin* Pin, const FString& Value)
{
	if (Pin)
		Pin->DefaultValue = Value;
}

void FHyperNodeCompilerHandler::PinDefaultValueInNode(UK2Node* InNode, const FString PinName, const FString& Value)
{
	// 确保输入参数有效
	if (!InNode)
		return;

	// 获取函数节点指定名称的引脚
	UEdGraphPin* Pin = NodePin(InNode, PinName);

	// 设置引脚默认值
	PinDefaultValue(Pin, Value);
}

void FHyperNodeCompilerHandler::PinDefaultValueInAssignNode(UK2Node_AssignmentStatement* AssignNode, const FString& DefaultValue)
{
	if (AssignNode && !DefaultValue.IsEmpty())
		PinDefaultValue(AssignValuePin(AssignNode), DefaultValue);
}

void FHyperNodeCompilerHandler::PinDefaultValueInSelectNode(UK2Node_Select* SelectNode, const int32 OptionIndex, const FString& Value)
{
	// 确保输入参数有效
	if (!SelectNode)
		return;

	// 获取所有选项引脚
	TArray<UEdGraphPin*> OptionPins;
	SelectNode->GetOptionPins(OptionPins);

	// 检查索引是否有效
	if (!OptionPins.IsValidIndex(OptionIndex))
		return;

	// 设置指定选项引脚的默认值
	PinDefaultValue(OptionPins[OptionIndex], Value);
}

//================ Node.Spawner																					========

UK2Node_Knot* FHyperNodeCompilerHandler::SpawnKnotNode()
{
	// 创建Knot节点
	UK2Node_Knot* KnotNode = SpawnNode<UK2Node_Knot>();

	// 分配默认引脚
	KnotNode->AllocateDefaultPins();

	return KnotNode;
}

UK2Node_TemporaryVariable* FHyperNodeCompilerHandler::SpawnTempVarNode(
	const FName PinCategory,
	const FName PinSubCategory,
	UObject* PinSubCategoryObject,
	const FSimpleMemberReference& PinSubCategoryMemberReference,
	const FEdGraphTerminalType& PinValueType,
	EPinContainerType ContainerType)
{
	// 创建临时变量节点
	UK2Node_TemporaryVariable* TempVar = SpawnNode<UK2Node_TemporaryVariable>();

	// 设置变量类型
	FEdGraphPinType PinType;
	PinType.PinCategory = PinCategory;
	PinType.PinSubCategory = PinSubCategory;
	PinType.PinSubCategoryObject = PinSubCategoryObject;
	PinType.PinSubCategoryMemberReference = PinSubCategoryMemberReference;
	PinType.PinValueType = PinValueType;
	PinType.ContainerType = ContainerType;

	TempVar->VariableType = PinType;

	// 分配默认引脚
	TempVar->AllocateDefaultPins();

	return TempVar;
}

UK2Node_TemporaryVariable* FHyperNodeCompilerHandler::SpawnCopyTempVarNode(const UEdGraphPin* SourcePin)
{
	if (!SourcePin)
		return nullptr;

	UK2Node_TemporaryVariable* TempVar = SpawnNode<UK2Node_TemporaryVariable>();
	TempVar->VariableType = SourcePin->PinType;
	TempVar->AllocateDefaultPins();
	return TempVar;
}

UK2Node_AssignmentStatement* FHyperNodeCompilerHandler::SpawnAssignNode(UEdGraphPin* RefPin, const FString& DefaultValue)
{
	if (!RefPin)
		return nullptr;

	// 创建赋值节点
	UK2Node_AssignmentStatement* AssignNode = SpawnNode<UK2Node_AssignmentStatement>();
	AssignNode->AllocateDefaultPins();

	// 连接参考引脚到变量引脚
	Link(RefPin, AssignRefPin(AssignNode));

	// 重建节点以更新引脚
	AssignNode->ReconstructNode();
	PinDefaultValueInAssignNode(AssignNode, DefaultValue);

	return AssignNode;
}

UK2Node_IfThenElse* FHyperNodeCompilerHandler::SpawnBranchNode(const bool bCondition)
{
	// 创建分支节点
	UK2Node_IfThenElse* Branch = SpawnNode<UK2Node_IfThenElse>();
	Branch->AllocateDefaultPins();

	// 设置条件引脚的默认值
	PinDefaultValue(Branch->GetConditionPin(), bCondition ? "true" : "false");

	return Branch;
}

UK2Node_ExecutionSequence* FHyperNodeCompilerHandler::SpawnSequenceNode(int32 ThenPinAmount)
{
	// 验证引脚数量
	if(ThenPinAmount < 2) ThenPinAmount = 2;

	// 创建序列节点
	UK2Node_ExecutionSequence* Sequence = SpawnNode<UK2Node_ExecutionSequence>();
	Sequence->AllocateDefaultPins();

	// 添加额外的输入引脚
	for(int32 i = 0; i < ThenPinAmount - 2; ++i)
		Sequence->AddInputPin();

	return Sequence;
}

UK2Node_SwitchEnum* FHyperNodeCompilerHandler::SpawnSwitchEnumNode(UEnum* SwitchEnum)
{
	UK2Node_SwitchEnum* Switch = SpawnNode<UK2Node_SwitchEnum>();
	Switch->Enum = SwitchEnum;
	Switch->AllocateDefaultPins();
	return Switch;
}

UK2Node_Select* FHyperNodeCompilerHandler::SpawnSelectNode(UEdGraphPin* TargetPin, UEdGraphPin* IndexPin, int32 PinAmount)
{
	// 验证引脚数量（最小为2）
	PinAmount = FMath::Max(2, PinAmount);

	// 创建节点
	UK2Node_Select* SelectNode = SpawnNode<UK2Node_Select>();
	SelectNode->AllocateDefaultPins();

	// 连接索引引脚
	Link(IndexPin, SelectNode->GetIndexPin());

	// 如果目标引脚存在，则连接
	if (TargetPin)
		Link(SelectNode->GetReturnValuePin(), TargetPin);

	// 添加额外的输入引脚
	for(int32 i = 0; i < PinAmount - 2; ++i)
		SelectNode->AddInputPin();

	// 重新构建节点以更新引脚
	SelectNode->ReconstructNode();

	return SelectNode;
}

UK2Node_MakeMap* FHyperNodeCompilerHandler::SpawnMakeMapNode(UEdGraphPin* TargetPin, int32 PairCount)
{
	// 验证引脚数量（最小为1对键值）
	PairCount = FMath::Max(1, PairCount);

	// 创建MakeMap节点
	UK2Node_MakeMap* MakeMapNode = SpawnNode<UK2Node_MakeMap>();
	MakeMapNode->AllocateDefaultPins();

	// 如果目标引脚存在，则连接并同步类型
	if (TargetPin)
		Link(MakeMapNode->GetOutputPin(), TargetPin);

	// 为每个额外的键值对添加引脚
	for(int32 i = 0; i < PairCount - 1; ++i)
		MakeMapNode->AddInputPin();

	// 重新构建节点以更新引脚
	MakeMapNode->ReconstructNode();

	return MakeMapNode;
}

UK2Node_MakeArray* FHyperNodeCompilerHandler::SpawnMakeArrayNode(UEdGraphPin* TargetPin, int32 ElementCount)
{
	// 验证元素数量（最小为1个元素）
	ElementCount = FMath::Max(1, ElementCount);

	// 创建MakeArray节点
	UK2Node_MakeArray* MakeArrayNode = SpawnNode<UK2Node_MakeArray>();
	MakeArrayNode->AllocateDefaultPins();

	// 如果目标引脚存在，则连接并同步类型
	if (TargetPin)
		Link(MakeArrayNode->GetOutputPin(), TargetPin);
	
	// 为每个额外的元素添加引脚
	for(int32 i = 0; i < ElementCount - 1; ++i)
		MakeArrayNode->AddInputPin();

	// 重新构建节点以更新引脚
	MakeArrayNode->ReconstructNode();

	return MakeArrayNode;
}

UK2Node_CallFunction* FHyperNodeCompilerHandler::SpawnEqualNameNode()
{
	// 创建Equal (Name)节点
	UK2Node_CallFunction* EqualNode = SpawnNode<UK2Node_CallFunction>();
	EqualNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_NameName), UKismetMathLibrary::StaticClass());
	EqualNode->AllocateDefaultPins();
	return EqualNode;
}

UK2Node_CallFunction* FHyperNodeCompilerHandler::SpawnMakeNameNode(const FString& NameValue)
{
	// 创建Conv String to Name节点
	UK2Node_CallFunction* ConvStringToNameNode = SpawnNode<UK2Node_CallFunction>();
	ConvStringToNameNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Conv_StringToName), UKismetStringLibrary::StaticClass());
	ConvStringToNameNode->AllocateDefaultPins();

	// 设置默认值
	if (UEdGraphPin* InStringPin = ConvStringToNameNode->FindPin(TEXT("InString")))
		InStringPin->DefaultValue = NameValue;

	return ConvStringToNameNode;
}

//================ Node.Delegate																				========

bool FHyperNodeCompilerHandler::CreateAsyncDelegateBinding(UClass* DelegateOwnerClass, const FName& DelegateName,UEdGraphPin* TargetObjectPin, UEdGraphPin*& InOutLastThenPin, UEdGraphPin*& OutEventThenPin)
{
	if (!DelegateOwnerClass || DelegateName.IsNone() || !TargetObjectPin || !InOutLastThenPin)
		return false;

	// 查找委托属性
	FMulticastDelegateProperty* DelegateProperty = nullptr;
	for (TFieldIterator<FProperty> PropIt(DelegateOwnerClass); PropIt; ++PropIt)
	{
		if (FMulticastDelegateProperty* MCDelegateProp = CastField<FMulticastDelegateProperty>(*PropIt))
		{
			if (PropIt->GetFName() == DelegateName)
			{
				DelegateProperty = MCDelegateProp;
				break;
			}
		}
	}

	if (!DelegateProperty)
		return false;

	bool bIsErrorFree = true;

	// 1. 创建AddDelegate节点 (使用编译器SpawnIntermediateNode)
	UK2Node_AddDelegate* AddDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(HandlingNode, SourceGraph);
	AddDelegateNode->SetFromProperty(DelegateProperty, false, DelegateOwnerClass);
	AddDelegateNode->AllocateDefaultPins();

	// 2. 连接Target和执行流
	bIsErrorFree &= Schema->TryCreateConnection(AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Self), TargetObjectPin);
	bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
	InOutLastThenPin = AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

	// 3. 创建CustomEvent节点 (使用编译器GUID确保唯一性)
	UK2Node_CustomEvent* CustomEventNode = CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(HandlingNode, SourceGraph);
	CustomEventNode->CustomFunctionName = *FString::Printf(TEXT("%s_%s"),
		*DelegateName.ToString(), *CompilerContext.GetGuid(HandlingNode));
	CustomEventNode->AllocateDefaultPins();

	// 4. 手动创建委托连接（因为FBaseAsyncTaskHelper是protected）
	// 创建Self节点
	UK2Node_Self* SelfNode = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(HandlingNode, SourceGraph);
	SelfNode->AllocateDefaultPins();

	// 创建CreateDelegate节点
	UK2Node_CreateDelegate* CreateDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CreateDelegate>(HandlingNode, SourceGraph);
	CreateDelegateNode->AllocateDefaultPins();

	// 连接：AddDelegate.DelegatePin ← CreateDelegate.DelegateOut
	bIsErrorFree &= Schema->TryCreateConnection(AddDelegateNode->GetDelegatePin(), CreateDelegateNode->GetDelegateOutPin());
	// 连接：CreateDelegate.ObjectIn ← Self.Self
	bIsErrorFree &= Schema->TryCreateConnection(SelfNode->FindPinChecked(UEdGraphSchema_K2::PN_Self), CreateDelegateNode->GetObjectInPin());
	// 设置CreateDelegate指向CustomEvent函数
	CreateDelegateNode->SetFunction(CustomEventNode->GetFunctionName());

	// 5. 手动复制委托签名到CustomEvent
	UFunction* DelegateSignature = AddDelegateNode->GetDelegateSignature();
	if (DelegateSignature)
	{
		for (TFieldIterator<FProperty> PropIt(DelegateSignature); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
		{
			const FProperty* Param = *PropIt;
			if (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm))
			{
				FEdGraphPinType PinType;
				if (Schema->ConvertPropertyToPinType(Param, /*out*/ PinType))
					CustomEventNode->CreateUserDefinedPin(Param->GetFName(), PinType, EGPD_Output);
			}
		}
	}

	// 6. 设置输出执行引脚
	OutEventThenPin = CustomEventNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

	return bIsErrorFree;
}

bool FHyperNodeCompilerHandler::CreateAsyncSingleDelegateBinding(UClass* DelegateOwnerClass, const FName& DelegateName, UEdGraphPin* TargetObjectPin, UEdGraphPin*& InOutLastThenPin, UEdGraphPin*& OutEventThenPin)
{
	if (!DelegateOwnerClass || DelegateName.IsNone() || !TargetObjectPin || !InOutLastThenPin)
		return false;

	// 查找单播委托属性（FDelegateProperty）
	FDelegateProperty* DelegateProperty = nullptr;
	for (TFieldIterator<FProperty> PropIt(DelegateOwnerClass); PropIt; ++PropIt)
	{
		if (FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(*PropIt))
		{
			if (PropIt->GetFName() == DelegateName)
			{
				DelegateProperty = DelegateProp;
				break;
			}
		}
	}

	if (!DelegateProperty)
		return false;

	bool bIsErrorFree = true;

	// 1. 创建 CustomEvent 节点（使用编译器 GUID 确保唯一性）
	UK2Node_CustomEvent* CustomEventNode = CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(HandlingNode, SourceGraph);
	CustomEventNode->CustomFunctionName = *FString::Printf(TEXT("%s_%s"),
		*DelegateName.ToString(), *CompilerContext.GetGuid(HandlingNode));

	// 2. 手动复制委托签名到 CustomEvent（在 AllocateDefaultPins 之前）
	UFunction* DelegateSignature = DelegateProperty->SignatureFunction;
	if (DelegateSignature)
	{
		for (TFieldIterator<FProperty> PropIt(DelegateSignature); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
		{
			const FProperty* Param = *PropIt;

			// 跳过返回值（ReturnValue），但包含所有输入参数和引用参数
			if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				continue;
			}

			// 跳过纯输出参数（有 OutParm 但没有 ReferenceParm）
			const bool bIsOutParam = Param->HasAnyPropertyFlags(CPF_OutParm);
			const bool bIsReferenceParam = Param->HasAnyPropertyFlags(CPF_ReferenceParm);

			// 只有纯输出参数（OutParm 但非 ReferenceParm）才跳过
			if (bIsOutParam && !bIsReferenceParam)
			{
				continue;
			}

			// 创建引脚
			FEdGraphPinType PinType;
			if (Schema->ConvertPropertyToPinType(Param, /*out*/ PinType))
			{
				// 引用参数必须设置 bIsReference 标志
				if (bIsReferenceParam)
				{
					PinType.bIsReference = true;
				}

				CustomEventNode->CreateUserDefinedPin(Param->GetFName(), PinType, EGPD_Output);
			}
			else
			{
				// 如果转换失败，记录错误
				CompilerContext.MessageLog.Warning(
					*FString::Printf(TEXT("Failed to convert parameter '%s' of type '%s' for delegate '%s'. @@"),
						*Param->GetName(),
						*Param->GetCPPType(),
						*DelegateName.ToString()),
					HandlingNode
				);
			}
		}
	}

	// 3. 分配 CustomEvent 的引脚
	CustomEventNode->AllocateDefaultPins();

	// 4. 再次验证并修正引用参数的引脚标志（AllocateDefaultPins 可能会重置某些标志）
	if (DelegateSignature)
	{
		for (TFieldIterator<FProperty> PropIt(DelegateSignature); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
		{
			const FProperty* Param = *PropIt;

			// 跳过返回值和纯输出参数
			if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				continue;
			}

			const bool bIsOutParam = Param->HasAnyPropertyFlags(CPF_OutParm);
			const bool bIsReferenceParam = Param->HasAnyPropertyFlags(CPF_ReferenceParm);

			if (bIsOutParam && !bIsReferenceParam)
			{
				continue;
			}

			// 如果是引用参数，确保对应的引脚设置了 bIsReference 标志
			if (bIsReferenceParam)
			{
				UEdGraphPin* Pin = CustomEventNode->FindPin(Param->GetFName(), EGPD_Output);
				if (Pin)
				{
					Pin->PinType.bIsReference = true;
				}
			}
		}
	}

	// 5. 创建 Self 节点
	UK2Node_Self* SelfNode = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(HandlingNode, SourceGraph);
	SelfNode->AllocateDefaultPins();

	// 6. 创建 CreateDelegate 节点并设置函数名（在 AllocateDefaultPins 之前）
	UK2Node_CreateDelegate* CreateDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CreateDelegate>(HandlingNode, SourceGraph);
	CreateDelegateNode->SetFunction(CustomEventNode->CustomFunctionName);
	CreateDelegateNode->AllocateDefaultPins();

	// 验证 SetFunction 是否成功
	if (CreateDelegateNode->GetFunctionName().IsNone())
	{
		CompilerContext.MessageLog.Error(
			*FString::Printf(TEXT("Failed to set function '%s' for CreateDelegate node. The CustomEvent signature may not match the delegate signature. Delegate: '%s' @@"),
				*CustomEventNode->CustomFunctionName.ToString(),
				*DelegateName.ToString()),
			HandlingNode
		);
		return false;
	}

	// 7. 连接：CreateDelegate.ObjectIn ← Self.Self
	bIsErrorFree &= Schema->TryCreateConnection(SelfNode->FindPinChecked(UEdGraphSchema_K2::PN_Self), CreateDelegateNode->GetObjectInPin());

	// 8. 创建 VariableSet 节点来设置委托属性
	UK2Node_VariableSet* SetDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_VariableSet>(HandlingNode, SourceGraph);
	SetDelegateNode->VariableReference.SetFromField<FProperty>(DelegateProperty, false);
	SetDelegateNode->AllocateDefaultPins();

	// 9. 连接执行流：InOutLastThenPin → SetDelegate.Execute
	bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, SetDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
	InOutLastThenPin = SetDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

	// 10. 连接：SetDelegate.Self ← TargetObject
	UEdGraphPin* SetDelegateSelfPin = SetDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Self);
	bIsErrorFree &= Schema->TryCreateConnection(TargetObjectPin, SetDelegateSelfPin);

	// 11. 连接：SetDelegate.Value ← CreateDelegate.DelegateOut
	UEdGraphPin* SetDelegateValuePin = nullptr;
	for (UEdGraphPin* Pin : SetDelegateNode->Pins)
	{
		if (Pin->Direction == EGPD_Input && Pin->PinName == DelegateName)
		{
			SetDelegateValuePin = Pin;
			break;
		}
	}

	if (SetDelegateValuePin)
	{
		bIsErrorFree &= Schema->TryCreateConnection(CreateDelegateNode->GetDelegateOutPin(), SetDelegateValuePin);
	}

	// 12. 设置输出执行引脚
	OutEventThenPin = CustomEventNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

	return bIsErrorFree;
}

void FHyperNodeCompilerHandler::CreateEventFilter(UEdGraphPin* EventThenPin, const FString& TargetName, UEdGraphPin* OutputPin)
{
	if (!EventThenPin || TargetName.IsEmpty() || !OutputPin)
		return;

	// 1. 创建MakeName节点，将TargetName转换为FName
	UK2Node_CallFunction* MakeNameNode = SpawnMakeNameNode(TargetName);

	// 2. 创建Equal节点比较PropertyName
	UK2Node_CallFunction* EqualNode = SpawnEqualNameNode();

	// 3. 创建Branch节点进行条件执行
	UK2Node_IfThenElse* BranchNode = SpawnBranchNode();

	// 4. 连接逻辑：
	// EventThenPin有一个PropertyName输出参数，我们需要找到它
	UEdGraphPin* PropertyNamePin = nullptr;
	if (EventThenPin->GetOwningNode())
	{
		// 在CustomEvent节点中查找PropertyName输出引脚
		for (UEdGraphPin* Pin : EventThenPin->GetOwningNode()->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && Pin->PinName == TEXT("PropertyName"))
			{
				PropertyNamePin = Pin;
				break;
			}
		}
	}

	if (PropertyNamePin)
	{
		// PropertyName → Equal.A
		Link(PropertyNamePin, FunctionInputPin(EqualNode, "A"));
		// MakeName.ReturnValue → Equal.B
		Link(FunctionReturnPin(MakeNameNode), FunctionInputPin(EqualNode, "B"));
		// Equal.ReturnValue → Branch.Condition
		Link(FunctionReturnPin(EqualNode), BranchConditionPin(BranchNode));
		// EventThenPin → Branch.Execute (修正：事件执行流应该经过Branch)
		Link(EventThenPin, ExecPin(BranchNode));
		// Branch.True → OutputPin (修正：条件满足时执行输出)
		Link(BranchTruePin(BranchNode), OutputPin);
		// Branch.False不连接，事件被过滤掉
	}
	else
	{
		// 如果找不到PropertyName引脚，直接连接（作为备用方案）
		Link(EventThenPin, OutputPin);
	}
}

//================ Node.Access																					========

//———————— Node.Common																						        ————

UEdGraphPin* FHyperNodeCompilerHandler::ProxyPin(const FName& PinName) const
{
	// 直接返回原始引脚，不再通过 Knot 代理
	return HandlingNode->FindPin(PinName);
}

UEdGraphPin* FHyperNodeCompilerHandler::ProxyExecPin() const
{
	// 获取并返回代理节点的执行输入引脚
	return ProxyPin(UEdGraphSchema_K2::PN_Execute);
}

UEdGraphPin* FHyperNodeCompilerHandler::ProxyThenPin() const
{
	// 获取并返回代理节点的执行输出引脚
	return ProxyPin(UEdGraphSchema_K2::PN_Then);
}

UEdGraphPin* FHyperNodeCompilerHandler::NodePin(UEdGraphNode* Node, const FString& PinName)
{
	return Node->FindPinChecked(*PinName);
}

UEdGraphPin* FHyperNodeCompilerHandler::ExecPin(UEdGraphNode* Node)
{
	// 确保输入参数有效
	if (!Node)
		return nullptr;

	// 获取并返回节点的执行输入引脚
	return Node->FindPinChecked(UEdGraphSchema_K2::PN_Execute);
}

UEdGraphPin* FHyperNodeCompilerHandler::ThenPin(UEdGraphNode* Node)
{
	// 确保输入参数有效
	if (!Node)
		return nullptr;

	// 获取并返回节点的执行输出引脚
	return Node->FindPinChecked(UEdGraphSchema_K2::PN_Then);
}

//———————— Node.Variable																					        ————

UEdGraphPin* FHyperNodeCompilerHandler::TempValuePin(UK2Node_TemporaryVariable* TempVar)
{
	// 确保输入参数有效
	if (!TempVar)
		return nullptr;

	// 获取并返回临时变量节点的变量引脚
	return TempVar->GetVariablePin();
}

//———————— Node.Assignment																					        ————

UEdGraphPin* FHyperNodeCompilerHandler::AssignRefPin(UK2Node_AssignmentStatement* AssignNode)
{
	// 确保输入参数有效
	if (!AssignNode)
		return nullptr;

	// 获取并返回赋值节点的引用引脚
	return AssignNode->GetVariablePin();
}

UEdGraphPin* FHyperNodeCompilerHandler::AssignValuePin(UK2Node_AssignmentStatement* AssignNode)
{
	// 确保输入参数有效
	if (!AssignNode)
		return nullptr;

	// 获取并返回赋值节点的值引脚
	return AssignNode->GetValuePin();
}

//———————— Node.Branch																						        ————

UEdGraphPin* FHyperNodeCompilerHandler::BranchConditionPin(UK2Node_IfThenElse* BranchNode)
{
	// 确保输入参数有效
	if (!BranchNode)
		return nullptr;

	// 获取并返回分支节点的条件引脚
	return BranchNode->GetConditionPin();
}

UEdGraphPin* FHyperNodeCompilerHandler::BranchTruePin(UK2Node_IfThenElse* BranchNode)
{
	// 确保输入参数有效
	if (!BranchNode)
		return nullptr;

	// 获取并返回分支节点的True引脚
	return BranchNode->GetThenPin();
}

UEdGraphPin* FHyperNodeCompilerHandler::BranchFalsePin(UK2Node_IfThenElse* BranchNode)
{
	// 确保输入参数有效
	if (!BranchNode)
		return nullptr;

	// 获取并返回分支节点的False引脚
	return BranchNode->GetElsePin();
}

//———————— Node.Sequence																					        ————

UEdGraphPin* FHyperNodeCompilerHandler::SequenceOutPin(UK2Node_ExecutionSequence* SequenceNode, const int32 Index)
{
	// 确保输入参数有效
	if (!SequenceNode)
		return nullptr;

	// 获取当前节点的最大输出索引
	const int32 CurrentMaxIndex = SequenceNode->Pins.Num() - 2;

	// 如果请求的索引大于当前最大索引，则扩展节点
	if (Index > CurrentMaxIndex)
	{
		// 计算需要增加的引脚数量
		const int32 PinsToAdd = Index - CurrentMaxIndex;

		// 循环添加所需的引脚
		for (int32 i = 0; i < PinsToAdd; ++i)
			SequenceNode->AddInputPin();
	}

	// 返回请求索引的输出引脚
	return SequenceNode->GetThenPinGivenIndex(Index);
}

//———————— Node.Switch																						        ————

UEdGraphPin* FHyperNodeCompilerHandler::SwitchEnumSelectionPin(UK2Node_SwitchEnum* SwitchNode) const
{
	// 确保输入参数有效
	if (!SwitchNode)
		return nullptr;

	// 获取并返回Switch节点的选择输入引脚
	return SwitchNode->GetSelectionPin();
}

UEdGraphPin* FHyperNodeCompilerHandler::SwitchEnumOutPin(UK2Node_SwitchEnum* SwitchNode, const FString& EnumValue)
{
	// 确保输入参数有效
	if (!SwitchNode)
		return nullptr;

	// 获取并返回Switch节点的指定枚举值对应的输出引脚
	return SwitchNode->FindPinChecked(*EnumValue);
}

//———————— Node.Select																						        ————

UEdGraphPin* FHyperNodeCompilerHandler::SelectOptionPin(UK2Node_Select* SelectNode, int32 OptionIndex)
{
	// 确保输入参数有效
	if (!SelectNode)
		return nullptr;

	// 获取所有选项引脚
	TArray<UEdGraphPin*> OptionPins;
	SelectNode->GetOptionPins(OptionPins);

	// 检查索引是否有效并返回对应引脚
	return OptionPins.IsValidIndex(OptionIndex) ? OptionPins[OptionIndex] : nullptr;
}

UEdGraphPin* FHyperNodeCompilerHandler::SelectResultPin(UK2Node_Select* SelectNode)
{
	// 确保输入参数有效
	if (!SelectNode)
		return nullptr;

	// 获取并返回Select节点的结果引脚
	return SelectNode->GetReturnValuePin();
}

//———————— Node.Map																							        ————

UEdGraphPin* FHyperNodeCompilerHandler::MakeMapKeyPin(UK2Node_MakeMap* MakeMapNode, int32 Index)
{
	// 确保输入参数有效
	if (!MakeMapNode)
		return nullptr;

	// 获取所有键和值引脚
	TArray<UEdGraphPin*> KeyPins;
	TArray<UEdGraphPin*> ValuePins;
	MakeMapNode->GetKeyAndValuePins(KeyPins, ValuePins);

	// 检查索引是否有效
	if (!KeyPins.IsValidIndex(Index))
		return nullptr;

	return KeyPins[Index];
}

UEdGraphPin* FHyperNodeCompilerHandler::MakeMapValuePin(UK2Node_MakeMap* MakeMapNode, int32 Index)
{
	// 确保输入参数有效
	if (!MakeMapNode)
		return nullptr;

	// 获取所有键和值引脚
	TArray<UEdGraphPin*> KeyPins;
	TArray<UEdGraphPin*> ValuePins;
	MakeMapNode->GetKeyAndValuePins(KeyPins, ValuePins);

	// 检查索引是否有效
	if (!ValuePins.IsValidIndex(Index))
		return nullptr;

	return ValuePins[Index];
}

UEdGraphPin* FHyperNodeCompilerHandler::MakeMapResultPin(UK2Node_MakeMap* MakeMapNode)
{
	// 确保输入参数有效
	if (!MakeMapNode)
		return nullptr;

	// 获取并返回Map输出引脚
	return MakeMapNode->GetOutputPin();
}

//———————— Node.Array																						        ————

UEdGraphPin* FHyperNodeCompilerHandler::MakeArrayElementPin(UK2Node_MakeArray* MakeArrayNode, int32 Index)
{
	// 确保输入参数有效
	if (!MakeArrayNode)
		return nullptr;

	// 获取所有输入引脚（对于数组，所有输入都是KeyPins，ValuePins为空）
	TArray<UEdGraphPin*> KeyPins;
	TArray<UEdGraphPin*> ValuePins;
	MakeArrayNode->GetKeyAndValuePins(KeyPins, ValuePins);

	// 检查索引是否有效
	if (!KeyPins.IsValidIndex(Index))
		return nullptr;

	return KeyPins[Index];
}

UEdGraphPin* FHyperNodeCompilerHandler::MakeArrayReturnPin(UK2Node_MakeArray* MakeArrayNode)
{
	// 确保输入参数有效
	if (!MakeArrayNode)
		return nullptr;

	// 获取并返回Array输出引脚
	return MakeArrayNode->GetOutputPin();
}

//———————— Node.CallFunction																				        ————

UEdGraphPin* FHyperNodeCompilerHandler::FunctionTargetPin(UK2Node_CallFunction* FunctionNode)
{
	// 确保输入参数有效
	if (!FunctionNode)
		return nullptr;

	// 获取并返回函数节点的Target引脚(名为"Self")
	return NodePin(FunctionNode, TEXT("Self"));
}

UEdGraphPin* FHyperNodeCompilerHandler::FunctionInputPin(UK2Node_CallFunction* FunctionNode, const FString& PinName)
{
	// 确保输入参数有效
	if (!FunctionNode)
		return nullptr;

	// 如果PinName为空,获取第一个非执行的输入引脚
	if (PinName.IsEmpty())
	{
		for (UEdGraphPin* Pin : FunctionNode->Pins)
		{
			if (Pin->Direction == EGPD_Input &&Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
				return Pin;
		}
		return nullptr;
	}

	// 获取并返回函数节点的指定输入引脚
	return NodePin(FunctionNode, PinName);
}

UEdGraphPin* FHyperNodeCompilerHandler::FunctionReturnPin(UK2Node_CallFunction* FunctionNode, const FString& PinName)
{
	// 确保输入参数有效
	if (!FunctionNode)
		return nullptr;

	// 如果PinName为空,获取返回值引脚
	if (PinName.IsEmpty())
		return FunctionNode->GetReturnValuePin();

	// 获取并返回函数节点的指定引脚
	return NodePin(FunctionNode, PinName);
}

//================ Link.Helpers																					========

void FHyperNodeCompilerHandler::HandleOriginalToIntermediate(UEdGraphPin* OriginalPin, UEdGraphPin* IntermediatePin)
{
	// 同步类型（对于动态类型引脚很重要）
	SyncPinTypeIfNeeded(OriginalPin, IntermediatePin);

	EPinUsageState* State = OriginalPinUsageState.Find(OriginalPin->PinName);
	if (!State)
		return;

	if (*State == EPinUsageState::Unused)
	{
		// 第一次使用：移动连接
		CompilerContext.MovePinLinksToIntermediate(*OriginalPin, *IntermediatePin);
		*State = EPinUsageState::Moved;
	}
	else
	{
		// 已经使用过：复制连接
		CompilerContext.CopyPinLinksToIntermediate(*OriginalPin, *IntermediatePin);
	}
}

void FHyperNodeCompilerHandler::HandleIntermediateToOriginal(UEdGraphPin* IntermediatePin, UEdGraphPin* OriginalPin)
{
	// 反向连接：Intermediate → Original（输出方向）
	// 同步类型：从 OriginalPin（具体类型）复制到 IntermediatePin（可能是Wildcard）
	SyncPinTypeIfNeeded(OriginalPin, IntermediatePin);

	EPinUsageState* State = OriginalPinUsageState.Find(OriginalPin->PinName);
	if (!State)
		return;

	if (*State == EPinUsageState::Unused)
	{
		CompilerContext.MovePinLinksToIntermediate(*OriginalPin, *IntermediatePin);
		*State = EPinUsageState::Moved;
	}
	else
	{
		CompilerContext.CopyPinLinksToIntermediate(*OriginalPin, *IntermediatePin);
	}
}

void FHyperNodeCompilerHandler::HandleDirectConnection(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
	// Intermediate 之间的直接连接
	AutoSyncPinType(SourcePin, TargetPin);

	// 使用Schema进行类型检查和连接验证
	if (Schema)
	{
		Schema->TryCreateConnection(SourcePin, TargetPin);
	}
	else
	{
		// 降级处理：如果Schema不可用，使用底层API
		SourcePin->MakeLinkTo(TargetPin);
	}
}

void FHyperNodeCompilerHandler::SyncPinTypeIfNeeded(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
	// 检查是否需要同步类型
	bool bShouldSync = false;

	// 情况1: 目标引脚是 Wildcard
	if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		bShouldSync = true;
	}
	// 情况2: 目标引脚是 Struct，但 SubCategoryObject 不同（动态类型引脚）
	else if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
	{
		// 如果源和目标的结构体类型不同，需要同步
		if (SourcePin->PinType.PinSubCategoryObject != TargetPin->PinType.PinSubCategoryObject)
		{
			bShouldSync = true;
		}
		// 如果引用标志不同，也需要同步
		else if (SourcePin->PinType.bIsReference != TargetPin->PinType.bIsReference)
		{
			bShouldSync = true;
		}
	}
	// 情况3: Map/Array/Set 容器类型 - 需要检查元素/值类型
	else if (TargetPin->PinType.ContainerType != EPinContainerType::None)
	{
		// 检查容器类型是否匹配
		if (TargetPin->PinType.ContainerType != SourcePin->PinType.ContainerType)
		{
			bShouldSync = true;
		}
		// 对于 Map 类型，需要额外检查 Value 类型（存储在 PinValueType 中）
		else if (TargetPin->PinType.ContainerType == EPinContainerType::Map)
		{
			// 检查 Map 的 Value 类型是否是 Wildcard
			if (TargetPin->PinType.PinValueType.TerminalCategory == UEdGraphSchema_K2::PC_Wildcard)
			{
				bShouldSync = true;
			}
			// 检查 Value 类型是否不匹配
			else if (TargetPin->PinType.PinValueType.TerminalCategory != SourcePin->PinType.PinValueType.TerminalCategory ||
					 TargetPin->PinType.PinValueType.TerminalSubCategory != SourcePin->PinType.PinValueType.TerminalSubCategory ||
					 TargetPin->PinType.PinValueType.TerminalSubCategoryObject != SourcePin->PinType.PinValueType.TerminalSubCategoryObject)
			{
				bShouldSync = true;
			}
		}
	}

	if (bShouldSync)
	{
		TargetPin->PinType = SourcePin->PinType;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

