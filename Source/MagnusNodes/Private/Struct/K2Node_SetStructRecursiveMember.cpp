#include "Struct/K2Node_SetStructRecursiveMember.h"

// Engine
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "KismetCompiler.h"
#include "NodeCompiler/Magnus_HyperNodeCompilerHandler.h"
#include "Styling/AppStyle.h"
#include "Struct/K2Node_SetStructMember.h"
#include "Struct/K2Node_GetStructMember.h"

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration																			========

//———————— Node.Appearance																							————

FText UK2Node_SetStructRecursiveMember::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (StructMemberReference.IsValid())
	{
		if (CachedNodeTitle.IsOutOfDate(this))
		{
			UScriptStruct* StructType = StructMemberReference.StructType.Get();
			int32 MemberCount = StructMemberReference.MemberPaths.Num();

			if (StructType)
			{
				if (MemberCount == 1)
				{
					CachedNodeTitle.SetCachedText(
						FText::FromString(FString::Printf(TEXT("Set %s.%s"),
							*StructType->GetDisplayNameText().ToString(),
							*StructMemberReference.MemberPaths[0])),
						this
					);
				}
				else
				{
					CachedNodeTitle.SetCachedText(
						FText::FromString(FString::Printf(TEXT("Set %s [%d members]"),
							*StructType->GetDisplayNameText().ToString(),
							MemberCount)),
						this
					);
				}
			}
			else
			{
				CachedNodeTitle.SetCachedText(
					FText::FromString(TEXT("SetStructMember (Invalid)")),
					this
				);
			}
		}
		return CachedNodeTitle;
	}

	return FText::FromString(TEXT("SetStructMember"));
}

FLinearColor UK2Node_SetStructRecursiveMember::GetNodeTitleColor() const
{
	return FLinearColor(1.0f, 0.4f, 0.2f);
}

FText UK2Node_SetStructRecursiveMember::GetTooltipText() const
{
	if (StructMemberReference.IsValid())
	{
		if (CachedTooltip.IsOutOfDate(this))
		{
			UScriptStruct* StructType = StructMemberReference.StructType.Get();
			int32 MemberCount = StructMemberReference.MemberPaths.Num();

			if (StructType)
			{
				CachedTooltip.SetCachedText(
					FText::FromString(FString::Printf(TEXT("Sets %d member(s) in '%s' (supports nested paths like 'Transform.Location.X')"),
						MemberCount,
						*StructType->GetDisplayNameText().ToString())),
					this
				);
			}
			else
			{
				CachedTooltip.SetCachedText(
					FText::FromString(TEXT("Sets members in a Struct (Invalid struct type)")),
					this
				);
			}
		}
		return CachedTooltip;
	}

	return FText::FromString(TEXT("Sets multiple members in a Struct (supports nested paths)\nSelect members in the Details panel"));
}

FText UK2Node_SetStructRecursiveMember::GetMenuCategory() const
{
	return FText::FromString(TEXT("Utilities|Struct Fields"));
}

FSlateIcon UK2Node_SetStructRecursiveMember::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor(1.0f, 0.4f, 0.2f);
	static FSlateIcon Icon("EditorStyle", "GraphEditor.MakeStruct_16x");
	return Icon;
}

//================ Pin.Management																				========

//———————— Pin.Construction																							————

void UK2Node_SetStructRecursiveMember::AllocateDefaultPins()
{
	// 创建执行引脚
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// 创建输入引脚：Struct (根据 MemberReference 确定类型)
	UEdGraphPin* StructInputPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, UK2Node_SetStructRecursiveMember::StructPinName());
	StructInputPin->PinType.bIsReference = true;

	Super::AllocateDefaultPins();

	// 根据 MemberReference 更新引脚类型
	OnMemberReferenceChanged();
}

//———————— Pin.Modification																							————

void UK2Node_SetStructRecursiveMember::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	// 在重建之前，保存旧的输入引脚连接信息
	// Key: 引脚名称, Value: 该引脚连接到的所有引脚
	TMap<FName, TArray<UEdGraphPin*>> OldPinConnections;
	for (UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin && OldPin->Direction == EGPD_Input && OldPin->LinkedTo.Num() > 0 &&
			OldPin->PinName != UEdGraphSchema_K2::PN_Execute && OldPin->PinName != StructPinName())
		{
			OldPinConnections.Add(OldPin->PinName, OldPin->LinkedTo);
		}
	}

	Super::ReallocatePinsDuringReconstruction(OldPins);

	// 重建后，恢复输入引脚的连接
	// 只有在引脚名称和类型都匹配的情况下才恢复连接
	for (const auto& Pair : OldPinConnections)
	{
		FName PinName = Pair.Key;
		const TArray<UEdGraphPin*>& LinkedPins = Pair.Value;

		// 查找新创建的同名引脚
		UEdGraphPin* NewPin = FindPin(PinName, EGPD_Input);
		if (NewPin && LinkedPins.Num() > 0)
		{
			// 恢复连接
			for (UEdGraphPin* LinkedPin : LinkedPins)
			{
				if (LinkedPin)
				{
					NewPin->MakeLinkTo(LinkedPin);
				}
			}
		}
	}
}

void UK2Node_SetStructRecursiveMember::PostReconstructNode()
{
	Super::PostReconstructNode();

	// 重建后更新引脚类型
	OnMemberReferenceChanged();
}

//———————— Pin.ValueChange																							————

void UK2Node_SetStructRecursiveMember::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
}

void UK2Node_SetStructRecursiveMember::PinTypeChanged(UEdGraphPin* Pin)
{
	Super::PinTypeChanged(Pin);
}

//———————— Pin.Connection																							————

bool UK2Node_SetStructRecursiveMember::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	// 限制输入引脚只能连接正确的结构体类型
	if (MyPin && MyPin->PinName == UK2Node_SetStructRecursiveMember::StructPinName())
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
					OutReason = FString::Printf(TEXT("Input must be of type '%s'."), *ExpectedStruct->GetDisplayNameText().ToString());
				}
				else
				{
					OutReason = TEXT("Struct type mismatch.");
				}
				return true;
			}
		}
	}

	// 检查值输入引脚（不允许连接 Wildcard）
	if (MyPin && MyPin->Direction == EGPD_Input && MyPin->PinName != StructPinName() && MyPin->PinName != UEdGraphSchema_K2::PN_Execute)
	{
		// 如果值输入引脚为 Wildcard，不允许连接
		if (MyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			OutReason = TEXT("Please select members in the Details panel first.");
			return true;
		}
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_SetStructRecursiveMember::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	// 当 Struct 引脚连接改变时，同步类型并更新 MemberReference
	if (Pin && Pin->PinName == UK2Node_SetStructRecursiveMember::StructPinName())
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

						// 验证现有的 MemberPaths 是否在新结构体中有效
						TArray<FString> ValidPaths;
						for (const FString& Path : StructMemberReference.MemberPaths)
						{
							FProperty* Property = StructMemberHelper::FindPropertyByRecursivePath(ConnectedStructType, Path);
							if (Property)
							{
								ValidPaths.Add(Path);
							}
						}

						// 如果有路径无效，更新 MemberPaths
						if (ValidPaths.Num() != StructMemberReference.MemberPaths.Num())
						{
							StructMemberReference.MemberPaths = ValidPaths;
						}

						// 更新输入引脚
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

				// 清空 MemberPaths（因为没有结构体类型了）
				StructMemberReference.MemberPaths.Empty();

				// 更新输入引脚
				OnMemberReferenceChanged();
			}

			GetGraph()->NotifyNodeChanged(this);
		}
	}
}

//———————— Pin.Reconstruct																							————

void UK2Node_SetStructRecursiveMember::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	if (!StructMemberReference.IsValid())
	{
		MessageLog.Error(TEXT("No members selected. Please select members in the Details panel. @@"), this);
		return;
	}

	UScriptStruct* StructType = StructMemberReference.StructType.Get();
	if (!StructType)
	{
		MessageLog.Error(TEXT("Struct type is invalid. @@"), this);
		return;
	}

	// 验证所有路径
	for (const FString& MemberPath : StructMemberReference.MemberPaths)
	{
		FProperty* Property = StructMemberHelper::FindPropertyByRecursivePath(StructType, MemberPath);
		if (!Property)
		{
			MessageLog.Error(*FString::Printf(TEXT("Member '%s' not found in Struct type '%s'. @@"),
				*MemberPath,
				*StructType->GetDisplayNameText().ToString()), this);
			continue;
		}

		if (!StructMemberHelper::CanCreatePinForProperty(Property))
		{
			MessageLog.Error(*FString::Printf(TEXT("Member '%s' type is not supported in Blueprints. @@"),
				*MemberPath), this);
		}
	}
}

//================ Struct.Access																				========

UScriptStruct* UK2Node_SetStructRecursiveMember::GetStructType() const
{
	// 只从 MemberReference 获取结构体类型
	return StructMemberReference.StructType.Get();
}

FProperty* UK2Node_SetStructRecursiveMember::GetPropertyFromPath(const FString& MemberPath) const
{
	UScriptStruct* StructType = GetStructType();
	if (!StructType)
	{
		return nullptr;
	}

	return StructMemberHelper::FindPropertyByRecursivePath(StructType, MemberPath);
}

void UK2Node_SetStructRecursiveMember::SetMemberPaths(const TArray<FString>& InMemberPaths)
{
	StructMemberReference.MemberPaths = InMemberPaths;
	OnMemberReferenceChanged();
}

void UK2Node_SetStructRecursiveMember::OnMemberReferenceChanged()
{
	// 更新 Struct 输入引脚类型
	if (UEdGraphPin* StructInputPin = FindPin(UK2Node_SetStructRecursiveMember::StructPinName()))
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

	// 在删除引脚之前，保存所有值输入引脚的连接信息
	TMap<FName, TArray<UEdGraphPin*>> OldPinConnections;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin && Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0 &&
			Pin->PinName != UEdGraphSchema_K2::PN_Execute && Pin->PinName != StructPinName())
		{
			OldPinConnections.Add(Pin->PinName, Pin->LinkedTo);
		}
	}

	// 移除所有值输入引脚（我们会按正确顺序重新创建）
	TArray<UEdGraphPin*> InputPinsToRemove;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin && Pin->Direction == EGPD_Input && Pin->PinName != UEdGraphSchema_K2::PN_Execute && Pin->PinName != StructPinName())
		{
			InputPinsToRemove.Add(Pin);
		}
	}

	for (UEdGraphPin* Pin : InputPinsToRemove)
	{
		Pin->BreakAllPinLinks();
		Pins.Remove(Pin);
	}

	bool bPinTypeChanged = (InputPinsToRemove.Num() > 0);

	// 按偏移量排序成员路径
	if (StructMemberReference.IsValid())
	{
		UScriptStruct* StructType = GetStructType();
		if (StructType)
		{
			// 创建路径和偏移索引的配对数组
			TArray<TPair<FString, TArray<int32>>> PathsWithOffsets;

			for (const FString& MemberPath : StructMemberReference.MemberPaths)
			{
				TArray<int32> OffsetIndices = StructMemberHelper::GetMemberOffsetIndices(StructType, MemberPath);
				if (OffsetIndices.Num() > 0)
				{
					PathsWithOffsets.Add(TPair<FString, TArray<int32>>(MemberPath, OffsetIndices));
				}
			}

			// 按偏移索引排序（深度优先）
			PathsWithOffsets.Sort([](const TPair<FString, TArray<int32>>& A, const TPair<FString, TArray<int32>>& B)
			{
				return StructMemberHelper::CompareOffsetIndices(A.Value, B.Value);
			});

			// 按排序后的顺序创建引脚
			const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

			for (const auto& Pair : PathsWithOffsets)
			{
				const FString& MemberPath = Pair.Key;
				FName PinName = InputPinName(MemberPath);
				FProperty* Property = GetPropertyFromPath(MemberPath);

				if (!Property || !StructMemberHelper::CanCreatePinForProperty(Property))
				{
					continue;
				}

				FEdGraphPinType NewPinType;
				if (Schema->ConvertPropertyToPinType(Property, /*out*/ NewPinType))
				{
					// 创建新引脚
					UEdGraphPin* NewPin = CreatePin(EGPD_Input, NewPinType.PinCategory, PinName);
					NewPin->PinType = NewPinType;
					NewPin->PinType.bIsReference = true;  // 使用引用传递，避免复制

					// 构建显示名称：将路径中的每一级都转换为 DisplayName
					FString DisplayPath;
					TArray<FString> PathParts;
					MemberPath.ParseIntoArray(PathParts, TEXT("."), true);
					const UScriptStruct* TempStructType = StructType;

					for (int32 PathIdx = 0; PathIdx < PathParts.Num(); ++PathIdx)
					{
						FProperty* PathProp = FindFProperty<FProperty>(TempStructType, FName(*PathParts[PathIdx]));
						if (PathProp)
						{
							if (!DisplayPath.IsEmpty())
							{
								DisplayPath += TEXT(".");
							}
							DisplayPath += PathProp->GetDisplayNameText().ToString();

							// 更新结构体类型用于下一级
							if (PathIdx < PathParts.Num() - 1)
							{
								FStructProperty* StructProp = CastField<FStructProperty>(PathProp);
								if (StructProp && StructProp->Struct)
								{
									TempStructType = StructProp->Struct;
								}
							}
						}
					}

					NewPin->PinFriendlyName = FText::FromString(DisplayPath.IsEmpty() ? MemberPath : DisplayPath);
					bPinTypeChanged = true;

					// 恢复该引脚的连接（如果旧引脚存在连接）
					if (TArray<UEdGraphPin*>* LinkedPins = OldPinConnections.Find(PinName))
					{
						for (UEdGraphPin* LinkedPin : *LinkedPins)
						{
							if (LinkedPin)
							{
								NewPin->MakeLinkTo(LinkedPin);
							}
						}
					}
				}
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

//================ Pin.Helpers																					========

FName UK2Node_SetStructRecursiveMember::InputPinName(const FString& MemberPath)
{
	// 将路径中的点号替换为下划线，避免引脚名称冲突
	FString SafeName = MemberPath.Replace(TEXT("."), TEXT("_"));
	return FName(*SafeName);
}

void UK2Node_SetStructRecursiveMember::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// 检查是否是 StructMemberReference 或其子属性发生了变化
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_SetStructRecursiveMember, StructMemberReference) ||
		MemberPropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_SetStructRecursiveMember, StructMemberReference))
	{
		// 立即更新引脚
		OnMemberReferenceChanged();
	}
}

//================ Blueprint.Integration																		========

//———————— Blueprint.Menu																							————

void UK2Node_SetStructRecursiveMember::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

//———————— Blueprint.Compile																				————

HNCH_StartExpandNode(UK2Node_SetStructRecursiveMember)

void Compile() override
{
	// 验证节点配置
	if (!OwnerNode->StructMemberReference.IsValid())
	{
		CompilerContext.MessageLog.Error(TEXT("No members selected @@"), OwnerNode);
		return;
	}

	UScriptStruct* StructType = OwnerNode->GetStructType();
	if (!StructType)
	{
		CompilerContext.MessageLog.Error(TEXT("Invalid struct type @@"), OwnerNode);
		return;
	}

	// 获取输入引脚
	UEdGraphPin* OriginalStructPin = ProxyPin(UK2Node_SetStructRecursiveMember::StructPinName());
	if (!OriginalStructPin || OriginalStructPin->LinkedTo.Num() == 0)
	{
		CompilerContext.MessageLog.Error(TEXT("Struct input must be connected @@"), OwnerNode);
		return;
	}

	// 获取执行引脚
	UEdGraphPin* ExecPin = ProxyExecPin();
	UEdGraphPin* ThenPin = ProxyThenPin();

	// 缓存中间节点，避免重复创建 Get 节点
	// Key: 路径前缀（例如 "Translation", "Translation.Location"）
	// Value: 该路径对应 Get 节点的输出引脚
	TMap<FString, UEdGraphPin*> PathPrefixToOutputPin;

	// 上一个节点的 Then 引脚，用于串联执行流
	UEdGraphPin* LastThenPin = ExecPin;

	// 为每个路径创建节点链
	for (const FString& MemberPath : OwnerNode->StructMemberReference.MemberPaths)
	{
		// 获取值输入引脚
		FName ValuePinName = UK2Node_SetStructRecursiveMember::InputPinName(MemberPath);
		UEdGraphPin* OriginalValuePin = ProxyPin(ValuePinName);
		if (!OriginalValuePin)
		{
			CompilerContext.MessageLog.Error(*FString::Printf(TEXT("Value pin for '%s' not found @@"), *MemberPath), OwnerNode);
			continue;
		}

		// 分割路径
		TArray<FString> PathParts;
		MemberPath.ParseIntoArray(PathParts, TEXT("."), true);

		if (PathParts.Num() == 0)
		{
			continue;
		}

		// 处理嵌套路径：为前面的部分创建 Get 节点，为最后一级创建 Set 节点
		UEdGraphPin* CurrentInputPin = OriginalStructPin->LinkedTo[0];  // 原始连接的源引脚
		UScriptStruct* CurrentStructType = StructType;
		FString CurrentPathPrefix;  // 累积的路径前缀

		// 遍历路径的每一级
		for (int32 i = 0; i < PathParts.Num(); ++i)
		{
			const FString& PartName = PathParts[i];
			bool bIsLastPart = (i == PathParts.Num() - 1);

			// 构建当前路径前缀
			if (CurrentPathPrefix.IsEmpty())
			{
				CurrentPathPrefix = PartName;
			}
			else
			{
				CurrentPathPrefix = CurrentPathPrefix + TEXT(".") + PartName;
			}

			if (bIsLastPart)
			{
				// 最后一级：创建 SetStructMemberField 节点
				UK2Node_SetStructMember* SetMemberNode = SpawnNode<UK2Node_SetStructMember>();
				SetMemberNode->StructMemberReference.StructType = CurrentStructType;
				SetMemberNode->StructMemberReference.MemberPath = PartName;  // 只用单级名称
				SetMemberNode->AllocateDefaultPins();

				// 连接执行流
				UEdGraphPin* SetExecPin = SetMemberNode->FindPin(UEdGraphSchema_K2::PN_Execute);
				UEdGraphPin* SetThenPin = SetMemberNode->FindPin(UEdGraphSchema_K2::PN_Then);

				if (SetExecPin && LastThenPin)
				{
					Link(LastThenPin, SetExecPin);
				}

				if (SetThenPin)
				{
					LastThenPin = SetThenPin;
				}

				// 连接 Struct 输入
				UEdGraphPin* SetStructPin = SetMemberNode->FindPin(TEXT("Struct"));
				if (SetStructPin && CurrentInputPin)
				{
					Link(CurrentInputPin, SetStructPin);
				}

				// 连接值输入
				UEdGraphPin* SetValuePin = SetMemberNode->FindPin(TEXT("FieldValue"));
				if (SetValuePin && OriginalValuePin->LinkedTo.Num() > 0)
				{
					// 连接到源引脚
					Link(OriginalValuePin->LinkedTo[0], SetValuePin);
				}
			}
			else
			{
				// 中间级：创建或使用缓存的 GetStructMemberField 节点
				UEdGraphPin** CachedOutputPin = PathPrefixToOutputPin.Find(CurrentPathPrefix);
				UEdGraphPin* MemberOutputPin = nullptr;

				if (CachedOutputPin && *CachedOutputPin)
				{
					// 使用缓存的节点输出
					MemberOutputPin = *CachedOutputPin;

					// 更新当前结构体类型（用于下一级）
					FProperty* Prop = FindFProperty<FProperty>(CurrentStructType, FName(*PartName));
					if (Prop)
					{
						FStructProperty* StructProp = CastField<FStructProperty>(Prop);
						if (StructProp && StructProp->Struct)
						{
							CurrentStructType = StructProp->Struct;
						}
					}
				}
				else
				{
					// 创建新的 GetStructMemberField 节点
					UK2Node_GetStructMember* GetMemberNode = SpawnNode<UK2Node_GetStructMember>();
					GetMemberNode->StructMemberReference.StructType = CurrentStructType;
					GetMemberNode->StructMemberReference.MemberPath = PartName;  // 只用单级名称
					GetMemberNode->AllocateDefaultPins();

					// 连接输入
					UEdGraphPin* MemberStructPin = GetMemberNode->FindPin(TEXT("Struct"));
					if (MemberStructPin && CurrentInputPin)
					{
						Link(CurrentInputPin, MemberStructPin);
					}

					// 获取输出
					MemberOutputPin = GetMemberNode->FindPin(TEXT("FieldValue"));
					if (!MemberOutputPin)
					{
						CompilerContext.MessageLog.Error(*FString::Printf(TEXT("Missing output for '%s' @@"), *PartName), OwnerNode);
						break;
					}

					// 缓存该节点的输出引脚
					PathPrefixToOutputPin.Add(CurrentPathPrefix, MemberOutputPin);

					// 更新当前结构体类型
					FProperty* Prop = FindFProperty<FProperty>(CurrentStructType, FName(*PartName));
					if (Prop)
					{
						FStructProperty* StructProp = CastField<FStructProperty>(Prop);
						if (StructProp && StructProp->Struct)
						{
							CurrentStructType = StructProp->Struct;
						}
					}
				}

				// 准备下一级的输入
				CurrentInputPin = MemberOutputPin;
			}
		}
	}

	// 连接最后的 Then 引脚到输出
	if (LastThenPin && ThenPin)
	{
		Link(LastThenPin, ThenPin);
	}
}

HNCH_EndExpandNode(UK2Node_SetStructRecursiveMember)

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
