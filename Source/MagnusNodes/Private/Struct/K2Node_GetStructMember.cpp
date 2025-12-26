#include "Struct/K2Node_GetStructMember.h"

// Engine
#include "BPTerminal.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "KismetCompiler.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Pin.Management																				========

//———————— Pin.Construction																							————

void UK2Node_GetStructMember::AllocateDefaultPins()
{
	// 创建输入引脚：Struct (根据 MemberReference 确定类型)
	UEdGraphPin* StructInputPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, UK2Node_GetStructMember::StructPinName());
	StructInputPin->PinType.bIsReference = true;

	// 创建输出引脚：FieldValue (根据 MemberReference 确定类型)
	UEdGraphPin* FieldOutputPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, UK2Node_GetStructMember::MemberPinName());
	FieldOutputPin->PinType.bIsReference = true;  // 输出引脚必须是引用类型

	Super::AllocateDefaultPins();

	// 根据 MemberReference 更新引脚类型
	OnMemberReferenceChanged();
}

//———————— Pin.Modification																							————

void UK2Node_GetStructMember::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);
}

void UK2Node_GetStructMember::PostReconstructNode()
{
	Super::PostReconstructNode();

	// 重建后更新引脚类型
	OnMemberReferenceChanged();
}

//———————— Pin.ValueChange																							————

void UK2Node_GetStructMember::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
}

void UK2Node_GetStructMember::PinTypeChanged(UEdGraphPin* Pin)
{
	Super::PinTypeChanged(Pin);
}

//———————— Pin.Connection																							————

bool UK2Node_GetStructMember::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	// 如果是输出引脚且为 Wildcard，不允许连接
	if (MyPin && MyPin->PinName == MemberPinName() && MyPin->Direction == EGPD_Output)
	{
		if (MyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			OutReason = TEXT("Please select a member in the Details panel first.");
			return true;
		}
	}

	// 限制输入引脚只能连接正确的结构体类型
	if (MyPin && MyPin->PinName == StructPinName())
	{
		// 如果输入引脚已经有类型（从 MemberReference 设置），检查类型匹配
		if (MyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
		{
			if (OtherPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Struct ||
				OtherPin->PinType.PinSubCategoryObject != MyPin->PinType.PinSubCategoryObject)
			{
				UScriptStruct* ExpectedStruct = Cast<UScriptStruct>(MyPin->PinType.PinSubCategoryObject.Get());
				if (ExpectedStruct)
				{
					OutReason = FString::Printf(TEXT("Input must be of type '%s'."), *ExpectedStruct->GetName());
				}
				else
				{
					OutReason = TEXT("Struct type mismatch.");
				}
				return true;
			}
		}
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_GetStructMember::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	// 当 Struct 引脚连接改变时，同步类型并更新 MemberReference
	if (Pin && Pin->PinName == UK2Node_GetStructMember::StructPinName())
	{
		if (Pin->LinkedTo.Num() > 0)
		{
			// 有连接，同步类型
			UEdGraphPin* LinkedPin = Pin->LinkedTo[0];
			if (LinkedPin && LinkedPin->PinType.PinSubCategoryObject.IsValid())
			{
				UScriptStruct* ConnectedStructType = Cast<UScriptStruct>(LinkedPin->PinType.PinSubCategoryObject.Get());
				if (ConnectedStructType)
				{
					// 更新引脚类型
					Pin->PinType = LinkedPin->PinType;
					Pin->PinType.bIsReference = true;

					// 同步到 MemberReference.StructType
					if (StructMemberReference.StructType != ConnectedStructType)
					{
						StructMemberReference.StructType = ConnectedStructType;

						// 如果当前的 MemberPath 在新结构体中无效，清空它
						if (!StructMemberReference.MemberPath.IsEmpty())
						{
							FProperty* Property = StructMemberHelper::FindPropertyBySinglePath(ConnectedStructType, StructMemberReference.MemberPath);
							if (!Property)
							{
								StructMemberReference.MemberPath.Empty();
							}
						}

						// 更新输出引脚
						OnMemberReferenceChanged();
					}

					GetGraph()->NotifyNodeChanged(this);
				}
			}
		}
		else
		{
			// 断开连接，重置为 Wildcard 并清空 StructType
			Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			Pin->PinType.PinSubCategoryObject = nullptr;
			Pin->PinType.bIsReference = true;

			// 重置 StructMemberReference.StructType
			if (StructMemberReference.StructType.Get())
			{
				StructMemberReference.StructType = nullptr;

				// 清空 MemberPath（因为没有结构体类型了）
				StructMemberReference.MemberPath.Empty();

				// 更新输出引脚为 Wildcard
				OnMemberReferenceChanged();
			}

			GetGraph()->NotifyNodeChanged(this);
		}
	}
}

//———————— Pin.Reconstruct																							————

void UK2Node_GetStructMember::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	if (!StructMemberReference.IsValid())
	{
		MessageLog.Error(TEXT("No member selected. Please select a member in the Details panel. @@"), this);
		return;
	}

	UScriptStruct* StructType = StructMemberReference.StructType.Get();
	if (!StructType)
	{
		MessageLog.Error(TEXT("Struct type is invalid. @@"), this);
		return;
	}

	FProperty* Property = StructMemberHelper::FindPropertyBySinglePath(StructType, StructMemberReference.MemberPath);
	if (!Property)
	{
		MessageLog.Error(*FString::Printf(TEXT("Member '%s' not found in Struct type '%s' (only first-level members are supported). @@"),
			*StructMemberReference.MemberPath,
			*StructType->GetName()), this);
		return;
	}

	if (!StructMemberHelper::CanCreatePinForProperty(Property))
	{
		MessageLog.Error(*FString::Printf(TEXT("Member '%s' type is not supported in Blueprints. @@"),
			*StructMemberReference.MemberPath), this);
	}
}

//================ Struct.Access																				========

UScriptStruct* UK2Node_GetStructMember::GetStructType() const
{
	// 只从 MemberReference 获取结构体类型
	return StructMemberReference.StructType.Get();
}

FProperty* UK2Node_GetStructMember::GetPropertyFromMemberReference() const
{
	if (!StructMemberReference.IsValid())
	{
		return nullptr;
	}

	UScriptStruct* StructType = GetStructType();
	if (!StructType)
	{
		return nullptr;
	}

	// 使用 MemberPath 查找属性
	return StructMemberHelper::FindPropertyBySinglePath(StructType, StructMemberReference.MemberPath);
}

void UK2Node_GetStructMember::SetMemberPath(const FString& InMemberPath)
{
	StructMemberReference.MemberPath = InMemberPath;
	OnMemberReferenceChanged();
}

void UK2Node_GetStructMember::OnMemberReferenceChanged()
{
	// 更新输入引脚类型
	if (UEdGraphPin* StructInputPin = FindPin(StructPinName()))
	{
		UScriptStruct* StructType = GetStructType();

		// 如果引脚没有连接，根据 MemberReference 设置引脚类型
		if (StructInputPin->LinkedTo.Num() == 0)
		{
			if (StructType)
			{
				// 设置输入引脚为指定的结构体类型
				StructInputPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
				StructInputPin->PinType.PinSubCategoryObject = StructType;
				StructInputPin->PinType.bIsReference = true;
			}
			else
			{
				// 没有结构体类型，设置为 Wildcard
				StructInputPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
				StructInputPin->PinType.PinSubCategoryObject = nullptr;
				StructInputPin->PinType.bIsReference = true;
			}
		}
	}

	// 更新输出引脚类型
	UEdGraphPin* FieldOutputPin = FindPin(MemberPinName());
	if (!FieldOutputPin)
	{
		return;
	}

	bool bPinTypeChanged = false;

	// 如果 MemberReference 无效，设置输出为 Wildcard
	if (!StructMemberReference.IsValid())
	{
		if (FieldOutputPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
		{
			FieldOutputPin->BreakAllPinLinks();
			FieldOutputPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			FieldOutputPin->PinType.PinSubCategoryObject = nullptr;
			FieldOutputPin->PinFriendlyName = FText::GetEmpty();
			bPinTypeChanged = true;
		}
	}
	else
	{
		// 有 MemberReference，更新输出引脚类型
		FProperty* Property = GetPropertyFromMemberReference();
		if (Property && StructMemberHelper::CanCreatePinForProperty(Property))
		{
			const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
			FEdGraphPinType NewPinType;

			if (Schema->ConvertPropertyToPinType(Property, /*out*/ NewPinType))
			{
				// 检查类型是否改变
				if (FieldOutputPin->PinType != NewPinType)
				{
					// 类型不匹配，断开连接
					if (FieldOutputPin->LinkedTo.Num() > 0)
					{
						FieldOutputPin->BreakAllPinLinks();
					}

					FieldOutputPin->PinType = NewPinType;
					FieldOutputPin->PinType.bIsReference = true;  // 强制设置为引用类型
					bPinTypeChanged = true;
				}

				// 始终更新引脚显示名称（即使类型未改变）
				FText NewFriendlyName = FText::FromString(StructMemberReference.MemberPath);

				if (!FieldOutputPin->PinFriendlyName.EqualTo(NewFriendlyName))
				{
					FieldOutputPin->PinFriendlyName = NewFriendlyName;
					bPinTypeChanged = true; // 标记为需要刷新
				}
			}
		}
		else
		{
			// 属性无效，设置为 Wildcard
			if (FieldOutputPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
			{
				FieldOutputPin->BreakAllPinLinks();
				FieldOutputPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
				FieldOutputPin->PinType.PinSubCategoryObject = nullptr;
				FieldOutputPin->PinFriendlyName = FText::GetEmpty();
				bPinTypeChanged = true;
			}
		}
	}

	// 立即通知节点变化和蓝图修改
	if (bPinTypeChanged || GetStructType())
	{
		// 强制刷新节点显示
		GetGraph()->NotifyGraphChanged();

		// 通知蓝图结构变化
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

void UK2Node_GetStructMember::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// 检查是否是 StructMemberReference 或其子属性发生了变化
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_GetStructMember, StructMemberReference) ||
		MemberPropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_GetStructMember, StructMemberReference))
	{
		// 立即更新引脚
		OnMemberReferenceChanged();
	}
}

//================ Blueprint.Integration																		========

//———————— Blueprint.Compile																						————

class FKCHandler_GetStructMemberField : public FNodeHandlingFunctor
{
public:
	FKCHandler_GetStructMemberField(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	FBPTerminal* RegisterInputTerm(FKismetFunctionContext& Context, UK2Node_GetStructMember* Node)
	{
		check(Node);

		UScriptStruct* StructType = Node->GetStructType();
		if (!StructType)
		{
			CompilerContext.MessageLog.Error(TEXT("Unknown Struct type @@"), Node);
			return nullptr;
		}

		// 找到输入引脚
		UEdGraphPin* InputPin = Node->FindPin(UK2Node_GetStructMember::StructPinName());
		if (!InputPin)
		{
			CompilerContext.MessageLog.Error(TEXT("Missing input pin @@"), Node);
			return nullptr;
		}

		// 获取输入引脚的 Net
		UEdGraphPin* Net = FEdGraphUtilities::GetNetFromPin(InputPin);
		check(Net);

		FBPTerminal** FoundTerm = Context.NetMap.Find(Net);
		FBPTerminal* Term = FoundTerm ? *FoundTerm : nullptr;

		if (!Term)
		{
			// 不允许字面量
			if ((Net->Direction == EGPD_Input) && (Net->LinkedTo.Num() == 0))
			{
				CompilerContext.MessageLog.Error(TEXT("No input Struct connected @@"), Net);
				return nullptr;
			}
			// 标准注册 net
			else
			{
				Term = Context.CreateLocalTerminalFromPinAutoChooseScope(Net, Context.NetNameMap->MakeValidName(Net));
			}
			Context.NetMap.Add(Net, Term);
		}

		// 验证类型匹配
		UStruct* StructInTerm = Cast<UStruct>(Term->Type.PinSubCategoryObject.Get());
		if (!StructInTerm || !StructInTerm->IsChildOf(StructType))
		{
			CompilerContext.MessageLog.Error(TEXT("Struct types don't match @@"), Node);
		}

		return Term;
	}

	void RegisterOutputTerm(FKismetFunctionContext& Context, UK2Node_GetStructMember* Node, UScriptStruct* StructType, UEdGraphPin* OutputPin, FBPTerminal* ContextTerm)
	{
		if (!Node->StructMemberReference.IsValid())
		{
			CompilerContext.MessageLog.Error(TEXT("No member selected @@"), Node);
			return;
		}

		// 检查是否为嵌套路径（不支持）
		if (Node->StructMemberReference.MemberPath.Contains(TEXT(".")))
		{
			CompilerContext.MessageLog.Error(TEXT("Nested member paths are not supported. Only first-level members are allowed. @@"), Node);
			return;
		}

		FProperty* BoundProperty = StructMemberHelper::FindPropertyBySinglePath(StructType, Node->StructMemberReference.MemberPath);
		if (!BoundProperty)
		{
			CompilerContext.MessageLog.Error(*FString::Printf(TEXT("Member '%s' not found in Struct @@"),
				*Node->StructMemberReference.MemberPath), Node);
			return;
		}

		if (BoundProperty->HasAnyPropertyFlags(CPF_Deprecated) && OutputPin->LinkedTo.Num())
		{
			FString Message = FString::Printf(TEXT("@@ : Member '%s' of Struct '%s' is deprecated."),
				*BoundProperty->GetDisplayNameText().ToString(),
				*StructType->GetDisplayNameText().ToString());
			CompilerContext.MessageLog.Warning(*Message, OutputPin->GetOuter());
		}

		// 简单访问：直接第一级成员访问
		FBPTerminal* Term = Context.CreateLocalTerminalFromPinAutoChooseScope(OutputPin, OutputPin->PinName.ToString());
		Term->bPassedByReference = ContextTerm->bPassedByReference;  // 继承输入Terminal的引用属性
		Term->AssociatedVarProperty = BoundProperty;
		Context.NetMap.Add(OutputPin, Term);
		Term->Context = ContextTerm;

		if (BoundProperty->HasAnyPropertyFlags(CPF_BlueprintReadOnly))
		{
			Term->bIsConst = true;
		}
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* InNode) override
	{
		UK2Node_GetStructMember* Node = Cast<UK2Node_GetStructMember>(InNode);
		check(Node);

		UScriptStruct* StructType = Node->GetStructType();
		if (!StructType)
		{
			return;
		}

		if (FBPTerminal* StructContextTerm = RegisterInputTerm(Context, Node))
		{
			UEdGraphPin* OutputPin = Node->FindPin(UK2Node_GetStructMember::MemberPinName());
			if (OutputPin)
			{
				RegisterOutputTerm(Context, Node, StructType, OutputPin, StructContextTerm);
			}
		}
	}
};

FNodeHandlingFunctor* UK2Node_GetStructMember::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_GetStructMemberField(CompilerContext);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
