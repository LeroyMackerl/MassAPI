// Leroy Works & Ember, All Rights Reserved.

#include "OperationFragment/K2Node_GetMassFragment.h"
#include "MassAPIFuncLib.h"
#include "MassAPIStructs.h"
#include "MassEntityTypes.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "NodeCompiler/Magnus_HyperNodeCompilerHandler.h"
#include "Struct/K2Node_GetStructRecursiveMember.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace UK2Node_GetMembersFromFragmentHelper
{
	// DataSource类型到标题的映射
	const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,				TEXT("GetMassFragment") },
		{ EMassFragmentSourceDataType::EntityHandle,		TEXT("GetMassFragment-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,	TEXT("GetMassFragment-Template") }
	};

	// 4. Get - Icon Colors
	const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceIconColors =
	{
		{ EMassFragmentSourceDataType::None,                FLinearColor(0.0f, 1.0f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,        FLinearColor(0.0f, 1.0f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,  FLinearColor(0.0f, 1.0f, 0.0f, 1.0f) },
	};

	// 4. Get - Title Colors
	const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceTitleColors =
	{
		{ EMassFragmentSourceDataType::None,                FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,        FLinearColor(0.0f, 1.0f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,  FLinearColor(0.0f, 1.0f, 0.0f, 1.0f) },
	};
}

using namespace UK2Node_GetMembersFromFragmentHelper;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration																			========

//———————— Node.Appearance																							————

FText UK2Node_GetMassFragment::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_GetMassFragment::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Composition"));
}

FText UK2Node_GetMassFragment::GetTooltipText() const
{
	return FText::FromString(TEXT("Get members from a fragment."));
}

FSlateIcon UK2Node_GetMassFragment::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = DataSourceIconColors.FindRef(CachedDataSourceType);

	static FSlateIcon Icon("EditorStyle", "GraphEditor.BreakStruct_16x");
	return Icon;
}

FLinearColor UK2Node_GetMassFragment::GetNodeTitleColor() const
{
	return DataSourceTitleColors.FindRef(CachedDataSourceType);
}

//================ Pin.Management																				========

//———————— Pin.Construction																							————

void UK2Node_GetMassFragment::AllocateDefaultPins()
{
	// Pure节点，不需要执行引脚

	// 创建DataSource引脚（Wildcard，支持FEntityHandle和FEntityTemplateData，引用类型）
	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsReference = true;

	// 创建FragmentType引脚（用于类型选择）
	UEdGraphPin* FragmentTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, UScriptStruct::StaticClass(), FragmentTypePinName());
	FragmentTypePin->PinType.PinSubCategory = UEdGraphSchema_K2::PSC_Self;

	// 根据bGetMember决定创建哪些输出引脚
	if (bGetMember)
	{
		// 获取成员模式：不创建OutFragment引脚，稍后由OnMemberReferenceChanged创建成员输出引脚
	}
	else
	{
		// 标准模式：创建OutFragment引脚（动态类型，初始为Wildcard，引用类型）
		UEdGraphPin* OutFragmentPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, FragmentOutPinName());
		OutFragmentPin->PinType.bIsReference = true;
	}

	Super::AllocateDefaultPins();

	// 初始化DataSource类型
	UpdateDataSourceType();

	// 如果是获取成员模式，初始化成员引脚
	if (bGetMember)
	{
		OnMemberReferenceChanged();
	}
}

//———————— Pin.Modification																							————

void UK2Node_GetMassFragment::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
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
				// 显式确保引用标志被设置
				NewDataSourcePin->PinType.bIsReference = true;
			}
			break;
		}
	}

	// 更新缓存的DataSource类型
	UpdateDataSourceType();

	// 恢复Fragment类型
	UScriptStruct* OldFragmentStruct = nullptr;
	// Prioritize getting the type from the data pin itself, as this will be correct even if the pin was split.
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin && OldPin->PinName == FragmentOutPinName())
		{
			if (OldPin->PinType.PinSubCategoryObject.IsValid())
			{
				OldFragmentStruct = Cast<UScriptStruct>(OldPin->PinType.PinSubCategoryObject.Get());
			}
			break;
		}
	}

	// Fallback to the type selection pin if the data pin didn't have a type
	if (!OldFragmentStruct)
	{
		OldFragmentStruct = GetFragmentStructFromOldPins(OldPins);
	}

	// Apply the found struct to the new FragmentType pin
	if (OldFragmentStruct)
	{
		if (UEdGraphPin* FragmentTypePin = FindPin(FragmentTypePinName()))
		{
			FragmentTypePin->DefaultObject = OldFragmentStruct;
		}
	}
}

void UK2Node_GetMassFragment::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
	OnFragmentTypeChanged();
}

//———————— Pin.ValueChange																							————

void UK2Node_GetMassFragment::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
	if (Pin && Pin->PinName == FragmentTypePinName())
	{
		OnFragmentTypeChanged();
	}
}

//———————— Pin.Connection																							————

bool UK2Node_GetMassFragment::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
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

	// 限制FragmentType引脚只能连接有效的Fragment结构体
	if (MyPin && MyPin->PinName == FragmentTypePinName())
	{
		if (const UScriptStruct* PinStruct = Cast<UScriptStruct>(OtherPin->PinType.PinSubCategoryObject.Get()))
		{
			if (!IsValidFragmentStruct(PinStruct))
			{
				OutReason = TEXT("The provided struct must be a valid Mass struct for this node.");
				return true;
			}
		}
	}

	// 限制FragmentOut引脚：当FragmentType为None时，只能连接4种特定的Fragment类型
	if (MyPin && MyPin->PinName == FragmentOutPinName())
	{
		// 检查FragmentType引脚是否为None
		UScriptStruct* CurrentFragmentType = GetFragmentStruct();
		if (!CurrentFragmentType)
		{
			// FragmentType为None，检查连接的引脚类型
			if (OtherPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
			{
				// 不允许连接到Wildcard
				OutReason = TEXT("OutFragment cannot connect to Wildcard when FragmentType is None. Please select a specific Fragment type first.");
				return true;
			}

			// 检查是否为结构体类型
			if (const UScriptStruct* PinStruct = Cast<UScriptStruct>(OtherPin->PinType.PinSubCategoryObject.Get()))
			{
				// 检查是否是有效的Fragment类型
				if (!IsValidFragmentStruct(PinStruct))
				{
					OutReason = TEXT("OutFragment must be one of: FMassFragment, FMassChunkFragment, FMassSharedFragment, or FMassConstSharedFragment.");
					return true;
				}
			}
			else
			{
				// 不是结构体类型，拒绝连接
				OutReason = TEXT("OutFragment must be a valid Mass Fragment struct type.");
				return true;
			}
		}
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_GetMassFragment::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
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
				// 有连接，同步类型（保持引用标志）
				UEdGraphPin* LinkedPin = DataSourcePin->LinkedTo[0];
				if (LinkedPin && LinkedPin->PinType.PinSubCategoryObject.IsValid())
				{
					DataSourcePin->PinType = LinkedPin->PinType;
					DataSourcePin->PinType.bIsReference = true;
				}
			}
			else
			{
				// 断开连接，恢复为Wildcard（保持引用标志）
				DataSourcePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
				DataSourcePin->PinType.PinSubCategoryObject = nullptr;
				DataSourcePin->PinType.bIsReference = true;
			}

			// 更新缓存的类型
			UpdateDataSourceType();
			GetGraph()->NotifyNodeChanged(this);
		}
	}

	// 当FragmentType引脚连接改变时，更新Fragment输出引脚类型
	if (Pin && Pin->PinName == FragmentTypePinName())
	{
		if (Pin->LinkedTo.Num() == 0)
		{
			OnFragmentTypeChanged();
		}
	}

	// 当FragmentOut引脚连接改变时，自动更新FragmentType（如果当前为None）
	if (Pin && Pin->PinName == FragmentOutPinName())
	{
		UEdGraphPin* FragmentOutPin = FindPin(FragmentOutPinName());
		UEdGraphPin* FragmentTypePin = FindPin(FragmentTypePinName());

		if (FragmentOutPin && FragmentTypePin)
		{
			if (FragmentOutPin->LinkedTo.Num() > 0)
			{
				// 有连接，检查FragmentType是否为None
				UScriptStruct* CurrentFragmentType = GetFragmentStruct();
				if (!CurrentFragmentType)
				{
					// FragmentType为None，从连接的引脚获取类型并更新
					UEdGraphPin* LinkedPin = FragmentOutPin->LinkedTo[0];
					if (LinkedPin && LinkedPin->PinType.PinSubCategoryObject.IsValid())
					{
						if (UScriptStruct* ConnectedStruct = Cast<UScriptStruct>(LinkedPin->PinType.PinSubCategoryObject.Get()))
						{
							if (IsValidFragmentStruct(ConnectedStruct))
							{
								// 更新FragmentType引脚和FragmentOut引脚的类型（保持引用类型）
								FragmentTypePin->DefaultObject = ConnectedStruct;
								FragmentOutPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
								FragmentOutPin->PinType.PinSubCategoryObject = ConnectedStruct;
								FragmentOutPin->PinType.bIsReference = true;

								// 触发Fragment类型改变事件
								if (FragmentTypePin->GetOwningNode())
								{
									FragmentTypePin->GetOwningNode()->PinDefaultValueChanged(FragmentTypePin);
								}
							}
						}
					}
				}
				else
				{
					// FragmentType不为None，同步FragmentOut引脚类型（保持引用类型）
					if (FragmentOutPin->PinType.PinSubCategoryObject != CurrentFragmentType)
					{
						FragmentOutPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
						FragmentOutPin->PinType.PinSubCategoryObject = CurrentFragmentType;
						FragmentOutPin->PinType.bIsReference = true;
					}
				}
			}
			else
			{
				// 断开连接，FragmentOut恢复为Wildcard（如果FragmentType为None，保持引用类型）
				UScriptStruct* CurrentFragmentType = GetFragmentStruct();
				if (!CurrentFragmentType && FragmentOutPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
				{
					FragmentOutPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
					FragmentOutPin->PinType.PinSubCategoryObject = nullptr;
					FragmentOutPin->PinType.bIsReference = true;
					GetGraph()->NotifyNodeChanged(this);
				}
			}
		}
	}
}

//================ DataSource.Access																			========

void UK2Node_GetMassFragment::UpdateDataSourceType()
{
	UEdGraphPin* DataSourcePin = FindPin(DataSourcePinName());
	if (!DataSourcePin)
	{
		CachedDataSourceType = EMassFragmentSourceDataType::None;
		return;
	}

	UScriptStruct* DataSourceStruct = Cast<UScriptStruct>(DataSourcePin->PinType.PinSubCategoryObject.Get());

	if (DataSourceStruct == FEntityHandle::StaticStruct())
	{
		CachedDataSourceType = EMassFragmentSourceDataType::EntityHandle;
	}
	else if (DataSourceStruct == FEntityTemplateData::StaticStruct())
	{
		CachedDataSourceType = EMassFragmentSourceDataType::EntityTemplateData;
	}
	else
	{
		CachedDataSourceType = EMassFragmentSourceDataType::None;
	}
}

//================ Fragment.Access																				========

void UK2Node_GetMassFragment::OnFragmentTypeChanged()
{
	UEdGraphPin* FragmentOutPin = FindPin(FragmentOutPinName());
	UScriptStruct* SelectedStruct = GetFragmentStruct();

	bool bPinTypeChanged = false;

	// 如果是获取成员模式，更新StructMemberReference并调用OnMemberReferenceChanged
	if (bGetMember)
	{
		// 同步StructType到StructMemberReference
		if (StructMemberReference.StructType != SelectedStruct)
		{
			StructMemberReference.StructType = SelectedStruct;

			// 验证现有的MemberPaths是否在新结构体中有效
			if (SelectedStruct)
			{
				TArray<FString> ValidPaths;
				for (const FString& Path : StructMemberReference.MemberPaths)
				{
					FProperty* Property = StructMemberHelper::FindPropertyByRecursivePath(SelectedStruct, Path);
					if (Property)
					{
						ValidPaths.Add(Path);
					}
				}

				// 如果有路径无效，更新MemberPaths
				if (ValidPaths.Num() != StructMemberReference.MemberPaths.Num())
				{
					StructMemberReference.MemberPaths = ValidPaths;
				}
			}
			else
			{
				// SelectedStruct为None，清空MemberPaths
				StructMemberReference.MemberPaths.Empty();
			}

			// 更新成员输出引脚
			OnMemberReferenceChanged();
			bPinTypeChanged = true;
		}

		// 标准模式下不应该有FragmentOut引脚，确保隐藏
		if (FragmentOutPin)
		{
			if (!FragmentOutPin->bHidden)
			{
				FragmentOutPin->bHidden = true;
				FragmentOutPin->BreakAllPinLinks();
				bPinTypeChanged = true;
			}
		}
	}
	else
	{
		// 标准模式：处理FragmentOut引脚

		// 当FragmentType为None时，断开并隐藏FragmentOut引脚
		if (!SelectedStruct)
		{
			if (FragmentOutPin)
			{
				// 断开所有连接
				if (FragmentOutPin->LinkedTo.Num() > 0)
				{
					FragmentOutPin->BreakAllPinLinks();
					bPinTypeChanged = true;
				}

				// 隐藏引脚
				if (!FragmentOutPin->bHidden)
				{
					FragmentOutPin->bHidden = true;
					bPinTypeChanged = true;
				}

				// 恢复为Wildcard（保持引用类型）
				if (FragmentOutPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
				{
					FragmentOutPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
					FragmentOutPin->PinType.PinSubCategoryObject = nullptr;
					FragmentOutPin->PinType.bIsReference = true;
					bPinTypeChanged = true;
				}
			}
		}
		else if (FragmentOutPin && IsValidFragmentStruct(SelectedStruct))
		{
			// FragmentType不为None，显示并更新FragmentOut引脚类型

			// 取消隐藏引脚
			if (FragmentOutPin->bHidden)
			{
				FragmentOutPin->bHidden = false;
				bPinTypeChanged = true;
			}

			// 更新引脚类型（保持引用类型）
			if (FragmentOutPin->PinType.PinSubCategoryObject != SelectedStruct)
			{
				// 检查是否有连接，以及连接是否合法（结构体一致）
				bool bShouldBreakLinks = false;
				if (FragmentOutPin->LinkedTo.Num() > 0)
				{
					// 检查连接的引脚类型是否与新的SelectedStruct一致
					UEdGraphPin* LinkedPin = FragmentOutPin->LinkedTo[0];
					if (LinkedPin && LinkedPin->PinType.PinSubCategoryObject.IsValid())
					{
						UScriptStruct* LinkedStruct = Cast<UScriptStruct>(LinkedPin->PinType.PinSubCategoryObject.Get());
						// 如果连接的结构体与新的结构体不一致，才断开连接
						if (LinkedStruct != SelectedStruct)
						{
							bShouldBreakLinks = true;
						}
					}
					else
					{
						// 连接的引脚类型无效，断开连接
						bShouldBreakLinks = true;
					}
				}

				// 只有在需要时才断开连接
				if (bShouldBreakLinks)
				{
					FragmentOutPin->BreakAllPinLinks();
					bPinTypeChanged = true;
				}

				FragmentOutPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
				FragmentOutPin->PinType.PinSubCategoryObject = SelectedStruct;
				FragmentOutPin->PinType.bIsReference = true;
				bPinTypeChanged = true;
			}
		}
	}

	if (bPinTypeChanged)
	{
		GetGraph()->NotifyNodeChanged(this);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

bool UK2Node_GetMassFragment::IsValidFragmentStruct(const UScriptStruct* Struct) const
{
	if (!Struct)
	{
		return false;
	}

	// 检查是否是 4 种允许的 Mass Fragment 类型之一
	const bool bIsMassFragment = Struct->IsChildOf(FMassFragment::StaticStruct());
	const bool bIsChunkFragment = Struct->IsChildOf(FMassChunkFragment::StaticStruct());
	const bool bIsSharedFragment = Struct->IsChildOf(FMassSharedFragment::StaticStruct());
	const bool bIsConstSharedFragment = Struct->IsChildOf(FMassConstSharedFragment::StaticStruct());

	return bIsMassFragment || bIsChunkFragment || bIsSharedFragment || bIsConstSharedFragment;
}

UScriptStruct* UK2Node_GetMassFragment::GetFragmentStruct() const
{
	UEdGraphPin* FragmentTypePin = FindPin(FragmentTypePinName());
	if (FragmentTypePin && FragmentTypePin->DefaultObject != nullptr && FragmentTypePin->LinkedTo.Num() == 0)
	{
		return Cast<UScriptStruct>(FragmentTypePin->DefaultObject);
	}
	return nullptr;
}

UScriptStruct* UK2Node_GetMassFragment::GetFragmentStructFromOldPins(const TArray<UEdGraphPin*>& OldPins) const
{
	const UEdGraphPin* FragmentTypePin = nullptr;
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin->PinName == FragmentTypePinName())
		{
			FragmentTypePin = OldPin;
			break;
		}
	}

	if (FragmentTypePin && FragmentTypePin->DefaultObject != nullptr && FragmentTypePin->LinkedTo.Num() == 0)
	{
		return Cast<UScriptStruct>(FragmentTypePin->DefaultObject);
	}
	return nullptr;
}

//================ Member.Access																				========

void UK2Node_GetMassFragment::OnMemberReferenceChanged()
{
	if (!bGetMember)
	{
		return;  // 非获取成员模式，不处理
	}

	UScriptStruct* FragmentStruct = GetFragmentStruct();

	// 同步StructMemberReference的StructType
	if (StructMemberReference.StructType != FragmentStruct)
	{
		StructMemberReference.StructType = FragmentStruct;
	}

	// 在删除引脚之前，保存所有输出引脚的连接信息
	TMap<FName, TArray<UEdGraphPin*>> OldPinConnections;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin && Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0)
		{
			OldPinConnections.Add(Pin->PinName, Pin->LinkedTo);
		}
	}

	// 移除所有输出引脚（我们会按正确顺序重新创建）
	TArray<UEdGraphPin*> OutputPinsToRemove;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin && Pin->Direction == EGPD_Output)
		{
			OutputPinsToRemove.Add(Pin);
		}
	}

	for (UEdGraphPin* Pin : OutputPinsToRemove)
	{
		Pin->BreakAllPinLinks();
		Pins.Remove(Pin);
	}

	bool bPinTypeChanged = (OutputPinsToRemove.Num() > 0);

	// 按偏移量排序成员路径并创建引脚
	if (StructMemberReference.IsValid() && FragmentStruct)
	{
		// 创建路径和偏移索引的配对数组
		TArray<TPair<FString, TArray<int32>>> PathsWithOffsets;

		for (const FString& MemberPath : StructMemberReference.MemberPaths)
		{
			TArray<int32> OffsetIndices = StructMemberHelper::GetMemberOffsetIndices(FragmentStruct, MemberPath);
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
			FName PinName = OutputPinName(MemberPath);
			FProperty* Property = StructMemberHelper::FindPropertyByRecursivePath(FragmentStruct, MemberPath);

			if (!Property || !StructMemberHelper::CanCreatePinForProperty(Property))
			{
				continue;
			}

			FEdGraphPinType NewPinType;
			if (Schema->ConvertPropertyToPinType(Property, /*out*/ NewPinType))
			{
				// 创建新引脚
				UEdGraphPin* NewPin = CreatePin(EGPD_Output, NewPinType.PinCategory, PinName);
				NewPin->PinType = NewPinType;
				NewPin->PinFriendlyName = FText::FromString(MemberPath);
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

	// 立即通知节点变化和蓝图修改
	if (bPinTypeChanged || FragmentStruct)
	{
		// 强制刷新节点显示
		GetGraph()->NotifyGraphChanged();

		// 通知蓝图结构变化
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

FName UK2Node_GetMassFragment::OutputPinName(const FString& MemberPath)
{
	// 将路径中的点号替换为下划线，避免引脚名称冲突
	FString SafeName = MemberPath.Replace(TEXT("."), TEXT("_"));
	return FName(*SafeName);
}

//================ Editor.PropertyChange																	========

void UK2Node_GetMassFragment::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// 检查是否是bGetMember发生了变化
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_GetMassFragment, bGetMember))
	{
		// 重建节点引脚
		ReconstructNode();
	}
	// 检查是否是StructMemberReference或其子属性发生了变化
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_GetMassFragment, StructMemberReference) ||
		MemberPropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_GetMassFragment, StructMemberReference))
	{
		// 立即更新引脚
		OnMemberReferenceChanged();
	}
}

//================ Blueprint.Integration																		========

void UK2Node_GetMassFragment::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

HNCH_StartExpandNode(UK2Node_GetMassFragment)

virtual void Compile() override
{
	// 在编译前再次更新DataSourceType，确保类型正确
	OwnerNode->UpdateDataSourceType();

	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		CompilerContext.MessageLog.Error(TEXT("DataSource must be connected to either FEntityHandle or FEntityTemplateData. @@"), OwnerNode);
		return;
	}

	// 检查FragmentType是否为None
	UScriptStruct* FragmentStruct = OwnerNode->GetFragmentStruct();
	if (!FragmentStruct)
	{
		CompilerContext.MessageLog.Error(TEXT("FragmentType must be specified. Please select a valid Mass Fragment type. @@"), OwnerNode);
		return;
	}

	// 创建GetFragment函数调用节点
	UK2Node_CallFunction* GetterFunctionNode;
	FString GetterFunctionDataSourcePinName;

	switch (OwnerNode->CachedDataSourceType)
	{
		case EMassFragmentSourceDataType::EntityHandle:
			GetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, GetFragment_Entity_Unified);
			GetterFunctionDataSourcePinName = TEXT("EntityHandle");
			break;
		case EMassFragmentSourceDataType::EntityTemplateData:
			GetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, GetFragment_Template_Unified);
			GetterFunctionDataSourcePinName = TEXT("TemplateData");
			break;
		default:
			return;
	}

	// 连接DataSource和FragmentType参数引脚
	Link(ProxyPin(UK2Node_GetMassFragment::DataSourcePinName()), FunctionInputPin(GetterFunctionNode, GetterFunctionDataSourcePinName));
	Link(ProxyPin(UK2Node_GetMassFragment::FragmentTypePinName()), FunctionInputPin(GetterFunctionNode, TEXT("FragmentType")));

	// 根据bGetMember决定输出处理
	if (OwnerNode->bGetMember)
	{
		// 获取成员模式：创建UK2Node_GetStructRecursiveMemberField节点

		// 验证MemberPaths是否有效
		if (!OwnerNode->StructMemberReference.IsValid())
		{
			CompilerContext.MessageLog.Error(TEXT("No members selected. Please select members in the Details panel. @@"), OwnerNode);
			return;
		}

		// 创建UK2Node_GetStructRecursiveMemberField节点
		UK2Node_GetStructRecursiveMember* MemberNode = SpawnNode<UK2Node_GetStructRecursiveMember>();
		MemberNode->StructMemberReference.StructType = FragmentStruct;
		MemberNode->StructMemberReference.MemberPaths = OwnerNode->StructMemberReference.MemberPaths;
		MemberNode->AllocateDefaultPins();
		MemberNode->OnMemberReferenceChanged();  // 创建输出引脚

		// 连接：GetFragment.OutFragment → MemberNode.Struct
		Link(NodePin(GetterFunctionNode, TEXT("OutFragment")), NodePin(MemberNode, TEXT("Struct")));

		// 移动原始节点的成员输出引脚连接到MemberNode
		for (const FString& MemberPath : OwnerNode->StructMemberReference.MemberPaths)
		{
			FName PinName = UK2Node_GetMassFragment::OutputPinName(MemberPath);
			UEdGraphPin* OriginalOutputPin = ProxyPin(PinName);
			UEdGraphPin* MemberOutputPin = NodePin(MemberNode, PinName.ToString());

			if (OriginalOutputPin && MemberOutputPin)
			{
				Link(MemberOutputPin, OriginalOutputPin);
			}
		}
	}
	else
	{
		// 标准模式：直接输出Fragment引用
		Link(NodePin(GetterFunctionNode, TEXT("OutFragment")), ProxyPin(UK2Node_GetMassFragment::FragmentOutPinName()));
	}
}

HNCH_EndExpandNode(UK2Node_GetMassFragment)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
