// Leroy Works & Ember, All Rights Reserved.

#include "OperationFragment/K2Node_SetMassFragment.h"
#include "MassAPIFuncLib.h"
#include "MassAPIStructs.h"
#include "MassEntityTypes.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "NodeCompiler/Magnus_HyperNodeCompilerHandler.h"
#include "Struct/K2Node_SetStructRecursiveMember.h"
#include "K2Node_VariableSet.h"
#include "K2Node_TemporaryVariable.h"
#include "Kismet/KismetSystemLibrary.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace UK2Node_SetMembersInFragmentHelper
{
	// DataSource类型到标题的映射
	const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,				TEXT("SetMassFragment") },
		{ EMassFragmentSourceDataType::EntityHandle,		TEXT("SetMassFragment-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,	TEXT("SetMassFragment-Template") }
	};

	// 2. Set - Icon Colors
	const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceIconColors =
	{
		{ EMassFragmentSourceDataType::None,                FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,        FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,  FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
	};

	// 2. Set - Title Colors
	const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceTitleColors =
	{
		{ EMassFragmentSourceDataType::None,                FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,        FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,  FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
	};
}

using namespace UK2Node_SetMembersInFragmentHelper;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration																			========

//———————— Node.Appearance																							————

FText UK2Node_SetMassFragment::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return  FText::FromString(DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_SetMassFragment::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Composition"));
}

FText UK2Node_SetMassFragment::GetTooltipText() const
{
	return FText::FromString(TEXT("Set members in a fragment."));
}

FSlateIcon UK2Node_SetMassFragment::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = DataSourceIconColors.FindRef(CachedDataSourceType);

	static FSlateIcon Icon("EditorStyle", "GraphEditor.MakeStruct_16x");
	return Icon;
}

FLinearColor UK2Node_SetMassFragment::GetNodeTitleColor() const
{
	return DataSourceTitleColors.FindRef(CachedDataSourceType);
}

//================ Pin.Management																				========

//———————— Pin.Construction																							————

void UK2Node_SetMassFragment::AllocateDefaultPins()
{
	// 1. Standard Pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsReference = true;

	UEdGraphPin* FragmentTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, UScriptStruct::StaticClass(), FragmentTypePinName());
	FragmentTypePin->PinType.PinSubCategory = UEdGraphSchema_K2::PSC_Self;

	// 2. Fragment Input Pin (Standard Mode)
	// Create this BEFORE deferred pin so it appears above it.
	if (!bSetMember)
	{
		UEdGraphPin* FragmentInPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, FragmentInPinName());
		FragmentInPin->PinType.bIsReference = true;
	}

	// 3. Deferred Pin (Create LAST among inputs)
	UEdGraphPin* DeferredPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, UK2Node_SetMassFragment::DeferredPinName());
	DeferredPin->DefaultValue = TEXT("false");

	Super::AllocateDefaultPins();

	// 4. Initialize State
	UpdateDataSourceType();

	// 5. Create Member Pins (If in Set Member mode)
	if (bSetMember)
	{
		OnMemberReferenceChanged();
	}
}

//———————— Pin.Modification																							————

void UK2Node_SetMassFragment::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
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
				NewDataSourcePin->PinType.bIsReference = true; // 确保引用标志被保留
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
		if (OldPin && OldPin->PinName == FragmentInPinName())
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

void UK2Node_SetMassFragment::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
	OnFragmentTypeChanged();
}

//———————— Pin.ValueChange																							————

void UK2Node_SetMassFragment::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
	if (Pin && Pin->PinName == FragmentTypePinName())
	{
		OnFragmentTypeChanged();
	}
}

//———————— Pin.Connection																							————

bool UK2Node_SetMassFragment::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
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

	// 限制FragmentIn引脚：当FragmentType为None时，只能连接4种特定的Fragment类型
	if (MyPin && MyPin->PinName == FragmentInPinName())
	{
		// 检查FragmentType引脚是否为None
		UScriptStruct* CurrentFragmentType = GetFragmentStruct();
		if (!CurrentFragmentType)
		{
			// FragmentType为None，检查连接的引脚类型
			if (OtherPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
			{
				// 不允许连接到Wildcard
				OutReason = TEXT("InFragment cannot connect to Wildcard when FragmentType is None. Please connect a specific Fragment type first.");
				return true;
			}

			// 检查是否为结构体类型
			if (const UScriptStruct* PinStruct = Cast<UScriptStruct>(OtherPin->PinType.PinSubCategoryObject.Get()))
			{
				// 检查是否是有效的Fragment类型
				if (!IsValidFragmentStruct(PinStruct))
				{
					OutReason = TEXT("InFragment must be one of: FMassFragment, FMassChunkFragment, FMassSharedFragment, or FMassConstSharedFragment.");
					return true;
				}
			}
			else
			{
				// 不是结构体类型，拒绝连接
				OutReason = TEXT("InFragment must be a valid Mass Fragment struct type.");
				return true;
			}
		}
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_SetMassFragment::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
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
				// 有连接，同步类型
				UEdGraphPin* LinkedPin = DataSourcePin->LinkedTo[0];
				if (LinkedPin && LinkedPin->PinType.PinSubCategoryObject.IsValid())
				{
					DataSourcePin->PinType = LinkedPin->PinType;
					DataSourcePin->PinType.bIsReference = true; // 确保引用标志被保留
				}
			}
			else
			{
				// 断开连接，恢复为Wildcard
				DataSourcePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
				DataSourcePin->PinType.PinSubCategoryObject = nullptr;
				DataSourcePin->PinType.bIsReference = true; // 确保引用标志被保留
			}

			// 更新缓存的类型
			UpdateDataSourceType();
			GetGraph()->NotifyNodeChanged(this);
		}
	}

	// 当FragmentType引脚连接改变时，更新Fragment输入引脚类型
	if (Pin && Pin->PinName == FragmentTypePinName())
	{
		if (Pin->LinkedTo.Num() == 0)
		{
			OnFragmentTypeChanged();
		}
	}

	// 当FragmentIn引脚连接改变时，自动更新FragmentType（如果当前为None）
	if (Pin && Pin->PinName == FragmentInPinName())
	{
		UEdGraphPin* FragmentInPin = FindPin(FragmentInPinName());
		UEdGraphPin* FragmentTypePin = FindPin(FragmentTypePinName());

		if (FragmentInPin && FragmentTypePin)
		{
			if (FragmentInPin->LinkedTo.Num() > 0)
			{
				// 有连接，检查FragmentType是否为None
				UScriptStruct* CurrentFragmentType = GetFragmentStruct();
				if (!CurrentFragmentType)
				{
					// FragmentType为None，从连接的引脚获取类型并更新
					UEdGraphPin* LinkedPin = FragmentInPin->LinkedTo[0];
					if (LinkedPin && LinkedPin->PinType.PinSubCategoryObject.IsValid())
					{
						if (UScriptStruct* ConnectedStruct = Cast<UScriptStruct>(LinkedPin->PinType.PinSubCategoryObject.Get()))
						{
							if (IsValidFragmentStruct(ConnectedStruct))
							{
								// 更新FragmentType引脚和FragmentIn引脚的类型
								FragmentTypePin->DefaultObject = ConnectedStruct;
								FragmentInPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
								FragmentInPin->PinType.PinSubCategoryObject = ConnectedStruct;

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
					// FragmentType不为None，同步FragmentIn引脚类型
					if (FragmentInPin->PinType.PinSubCategoryObject != CurrentFragmentType)
					{
						FragmentInPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
						FragmentInPin->PinType.PinSubCategoryObject = CurrentFragmentType;
					}
				}
			}
			else
			{
				// 断开连接，FragmentIn恢复为Wildcard（如果FragmentType为None）
				UScriptStruct* CurrentFragmentType = GetFragmentStruct();
				if (!CurrentFragmentType && FragmentInPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
				{
					FragmentInPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
					FragmentInPin->PinType.PinSubCategoryObject = nullptr;
					GetGraph()->NotifyNodeChanged(this);
				}
			}
		}
	}
}

//================ DataSource.Access																			========

void UK2Node_SetMassFragment::UpdateDataSourceType()
{
	// Detection Logic
	UEdGraphPin* DataSourcePin = FindPin(DataSourcePinName());
	if (!DataSourcePin) { CachedDataSourceType = EMassFragmentSourceDataType::None; return; }

	UScriptStruct* DataSourceStruct = Cast<UScriptStruct>(DataSourcePin->PinType.PinSubCategoryObject.Get());
	if (DataSourceStruct == FEntityHandle::StaticStruct()) CachedDataSourceType = EMassFragmentSourceDataType::EntityHandle;
	else if (DataSourceStruct == FEntityTemplateData::StaticStruct()) CachedDataSourceType = EMassFragmentSourceDataType::EntityTemplateData;
	else CachedDataSourceType = EMassFragmentSourceDataType::None;

	// Visibility Logic
	UEdGraphPin* DeferredPin = FindPin(TEXT("bDeferred"));
	if (DeferredPin)
	{
		const bool bShow = (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle);
		if (DeferredPin->bHidden == bShow)
		{
			DeferredPin->bHidden = !bShow;
			if (!bShow) DeferredPin->BreakAllPinLinks();
		}
	}
}

//================ Fragment.Access																				========

void UK2Node_SetMassFragment::OnFragmentTypeChanged()
{
	UEdGraphPin* FragmentInPin = FindPin(FragmentInPinName());
	UScriptStruct* SelectedStruct = GetFragmentStruct();

	bool bPinTypeChanged = false;

	// 如果是设置成员模式，更新StructMemberReference并调用OnMemberReferenceChanged
	if (bSetMember)
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

			// 更新成员输入引脚
			OnMemberReferenceChanged();
			bPinTypeChanged = true;
		}

		// 设置成员模式下不应该有FragmentIn引脚，确保隐藏
		if (FragmentInPin)
		{
			if (!FragmentInPin->bHidden)
			{
				FragmentInPin->bHidden = true;
				FragmentInPin->BreakAllPinLinks();
				bPinTypeChanged = true;
			}
		}
	}
	else
	{
		// 标准模式：处理FragmentIn引脚

		// 当FragmentType为None时，断开并隐藏FragmentIn引脚
		if (!SelectedStruct)
		{
			if (FragmentInPin)
			{
				// 断开所有连接
				if (FragmentInPin->LinkedTo.Num() > 0)
				{
					FragmentInPin->BreakAllPinLinks();
					bPinTypeChanged = true;
				}

				// 隐藏引脚
				if (!FragmentInPin->bHidden)
				{
					FragmentInPin->bHidden = true;
					bPinTypeChanged = true;
				}

				// 恢复为Wildcard（保持引用类型）
				if (FragmentInPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
				{
					FragmentInPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
					FragmentInPin->PinType.PinSubCategoryObject = nullptr;
					FragmentInPin->PinType.bIsReference = true;
					bPinTypeChanged = true;
				}
			}
		}
		else if (FragmentInPin && IsValidFragmentStruct(SelectedStruct))
		{
			// FragmentType不为None，显示并更新FragmentIn引脚类型

			// [FIX 2] CHECK SPLIT STATUS
			// Only unhide if it's currently hidden AND it is NOT split (SubPins is empty).
			// If SubPins > 0, the pin was split by user and should remain hidden.
			if (FragmentInPin->bHidden && FragmentInPin->SubPins.Num() == 0)
			{
				FragmentInPin->bHidden = false;
				bPinTypeChanged = true;
			}

			// 更新引脚类型（保持引用类型）
			if (FragmentInPin->PinType.PinSubCategoryObject != SelectedStruct)
			{
				// 检查是否有连接，以及连接是否合法（结构体一致）
				bool bShouldBreakLinks = false;
				if (FragmentInPin->LinkedTo.Num() > 0)
				{
					// 检查连接的引脚类型是否与新的SelectedStruct一致
					UEdGraphPin* LinkedPin = FragmentInPin->LinkedTo[0];
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
					FragmentInPin->BreakAllPinLinks();
					bPinTypeChanged = true;
				}

				FragmentInPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
				FragmentInPin->PinType.PinSubCategoryObject = SelectedStruct;
				FragmentInPin->PinType.bIsReference = true;  // 使用引用传递，避免复制
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

bool UK2Node_SetMassFragment::IsValidFragmentStruct(const UScriptStruct* Struct) const
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

UScriptStruct* UK2Node_SetMassFragment::GetFragmentStruct() const
{
	UEdGraphPin* FragmentTypePin = FindPin(FragmentTypePinName());
	if (FragmentTypePin && FragmentTypePin->DefaultObject != nullptr && FragmentTypePin->LinkedTo.Num() == 0)
	{
		return Cast<UScriptStruct>(FragmentTypePin->DefaultObject);
	}
	return nullptr;
}

UScriptStruct* UK2Node_SetMassFragment::GetFragmentStructFromOldPins(const TArray<UEdGraphPin*>& OldPins) const
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

void UK2Node_SetMassFragment::OnMemberReferenceChanged()
{
	if (!bSetMember)
	{
		return;  // Not in Set Member mode, do nothing
	}

	UScriptStruct* FragmentStruct = GetFragmentStruct();

	// Sync struct type to StructMemberReference
	if (StructMemberReference.StructType != FragmentStruct)
	{
		StructMemberReference.StructType = FragmentStruct;
	}

	// --- 1. Preserve Connections AND Values ---
	// Before deleting pins, save connections for both Member pins AND the Deferred pin.
	// [FIX 1] We must also preserve the DefaultValue of the DeferredPin, 
	// because ReallocatePinsDuringReconstruction has already run and restored it, 
	// but we are about to destroy it again.

	// Map to store connections for Member pins (Key: PinName, Value: Connected Pins)
	TMap<FName, TArray<UEdGraphPin*>> OldMemberPinConnections;

	// Variables to store bDeferred state
	TArray<UEdGraphPin*> OldDeferredPinConnections;
	bool bWasDeferredPinVisible = true;
	FString SavedDeferredDefaultValue = TEXT("false"); // Default backup

	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			// Explicitly check for bDeferred pin to save its state.
			if (Pin->PinName == UK2Node_SetMassFragment::DeferredPinName())
			{
				OldDeferredPinConnections = Pin->LinkedTo;
				bWasDeferredPinVisible = !Pin->bHidden;
				SavedDeferredDefaultValue = Pin->DefaultValue; // [FIX 1] Capture Value
			}
			// Save other dynamic member pins (ignoring static pins like Exec, DataSource, FragmentType)
			else if (Pin->LinkedTo.Num() > 0 &&
				Pin->PinName != UEdGraphSchema_K2::PN_Execute &&
				Pin->PinName != DataSourcePinName() &&
				Pin->PinName != FragmentTypePinName())
			{
				OldMemberPinConnections.Add(Pin->PinName, Pin->LinkedTo);
			}
		}
	}

	// --- 2. Remove Dynamic Input Pins ---
	// We remove ALL dynamic input pins (including bDeferred) to ensure we can recreate them in the correct order.

	TArray<UEdGraphPin*> InputPinsToRemove;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin && Pin->Direction == EGPD_Input &&
			Pin->PinName != UEdGraphSchema_K2::PN_Execute &&
			Pin->PinName != DataSourcePinName() &&
			Pin->PinName != FragmentTypePinName())
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

	// --- 3. Recreate Member Pins ---
	// Sort members by memory offset and create pins for them.

	if (StructMemberReference.IsValid() && FragmentStruct)
	{
		// Create pairs of (Path, Offsets) for sorting
		TArray<TPair<FString, TArray<int32>>> PathsWithOffsets;

		for (const FString& MemberPath : StructMemberReference.MemberPaths)
		{
			TArray<int32> OffsetIndices = StructMemberHelper::GetMemberOffsetIndices(FragmentStruct, MemberPath);
			if (OffsetIndices.Num() > 0)
			{
				PathsWithOffsets.Add(TPair<FString, TArray<int32>>(MemberPath, OffsetIndices));
			}
		}

		// Sort by offset (Depth-First)
		PathsWithOffsets.Sort([](const TPair<FString, TArray<int32>>& A, const TPair<FString, TArray<int32>>& B)
			{
				return StructMemberHelper::CompareOffsetIndices(A.Value, B.Value);
			});

		// Create pins in sorted order
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		for (const auto& Pair : PathsWithOffsets)
		{
			const FString& MemberPath = Pair.Key;
			FName PinName = InputPinName(MemberPath);
			FProperty* Property = StructMemberHelper::FindPropertyByRecursivePath(FragmentStruct, MemberPath);

			if (!Property || !StructMemberHelper::CanCreatePinForProperty(Property))
			{
				continue;
			}

			FEdGraphPinType NewPinType;
			if (Schema->ConvertPropertyToPinType(Property, /*out*/ NewPinType))
			{
				// Create new pin (Reference type)
				UEdGraphPin* NewPin = CreatePin(EGPD_Input, NewPinType.PinCategory, PinName);
				NewPin->PinType = NewPinType;
				NewPin->PinType.bIsReference = true;  // Pass by reference
				NewPin->PinFriendlyName = FText::FromString(MemberPath);
				bPinTypeChanged = true;

				// Restore Member Connections if they existed
				if (TArray<UEdGraphPin*>* LinkedPins = OldMemberPinConnections.Find(PinName))
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

	// --- 4. Recreate Deferred Pin ---
	// We recreate it LAST so it appears at the bottom of the input list, below all members.
	UEdGraphPin* NewDeferredPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, UK2Node_SetMassFragment::DeferredPinName());
	NewDeferredPin->DefaultValue = SavedDeferredDefaultValue; // [FIX 1] Restore Value
	NewDeferredPin->bHidden = !bWasDeferredPinVisible; // Restore previous visibility state

	// Restore Deferred Connections
	for (UEdGraphPin* LinkedPin : OldDeferredPinConnections)
	{
		if (LinkedPin)
		{
			NewDeferredPin->MakeLinkTo(LinkedPin);
		}
	}

	// --- 5. Finalize ---
	// Ensure visibility logic is correct based on DataSourceType (Entity vs Template)
	UpdateDataSourceType();

	// Notify graph of changes
	if (bPinTypeChanged || FragmentStruct)
	{
		GetGraph()->NotifyGraphChanged();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

FName UK2Node_SetMassFragment::InputPinName(const FString& MemberPath)
{
	// 将路径中的点号替换为下划线，避免引脚名称冲突
	FString SafeName = MemberPath.Replace(TEXT("."), TEXT("_"));
	return FName(*SafeName);
}

//================ Editor.PropertyChange																	========

void UK2Node_SetMassFragment::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// 检查是否是bSetMember发生了变化
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_SetMassFragment, bSetMember))
	{
		// 重建节点引脚
		ReconstructNode();
	}
	// 检查是否是StructMemberReference或其子属性发生了变化
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_SetMassFragment, StructMemberReference) ||
		MemberPropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_SetMassFragment, StructMemberReference))
	{
		// 立即更新引脚
		OnMemberReferenceChanged();
	}
}

//================ Blueprint.Integration																		========

void UK2Node_SetMassFragment::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

HNCH_StartExpandNode(UK2Node_SetMassFragment)

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

	// 根据bSetMember决定编译方式
	if (OwnerNode->bSetMember)
	{
		// 设置成员模式：GetFragment → LocalWildcard → SetStructRecursiveMemberField → SetFragment

		// 验证MemberPaths是否有效
		if (!OwnerNode->StructMemberReference.IsValid())
		{
			CompilerContext.MessageLog.Error(TEXT("No members selected. Please select members in the Details panel. @@"), OwnerNode);
			return;
		}

		// 创建Knot节点用于分发DataSource和FragmentType到多个目标
		UK2Node_Knot* DataSourceKnot = SpawnKnotNode();
		UK2Node_Knot* FragmentTypeKnot = SpawnKnotNode();

		// 将原始引脚连接到Knot节点
		Link(ProxyPin(UK2Node_SetMassFragment::DataSourcePinName()), DataSourceKnot->GetInputPin());
		Link(ProxyPin(UK2Node_SetMassFragment::FragmentTypePinName()), FragmentTypeKnot->GetInputPin());

		// 1. 创建GetFragment函数调用节点
		UK2Node_CallFunction* GetterFunctionNode;
		FString GetterFunctionDataSourcePinName;
		FString SetterFunctionDataSourcePinName;

		switch (OwnerNode->CachedDataSourceType)
		{
		case EMassFragmentSourceDataType::EntityHandle:
			GetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, GetFragment_Entity_Unified);
			GetterFunctionDataSourcePinName = TEXT("EntityHandle");
			SetterFunctionDataSourcePinName = TEXT("EntityHandle");
			break;
		case EMassFragmentSourceDataType::EntityTemplateData:
			GetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, GetFragment_Template_Unified);
			GetterFunctionDataSourcePinName = TEXT("TemplateData");
			SetterFunctionDataSourcePinName = TEXT("TemplateData");
			break;
		default:
			return;
		}

		// 从Knot节点输出连接到GetFragment（先连接FragmentType让OutFragment确定类型）
		Link(DataSourceKnot->GetOutputPin(), FunctionInputPin(GetterFunctionNode, GetterFunctionDataSourcePinName));
		Link(FragmentTypeKnot->GetOutputPin(), FunctionInputPin(GetterFunctionNode, TEXT("FragmentType")));

		// 获取GetFragment的OutFragment引脚并手动设置类型
		UEdGraphPin* OutFragmentPin = NodePin(GetterFunctionNode, TEXT("OutFragment"));
		// 手动设置OutFragment的类型为FragmentStruct
		OutFragmentPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		OutFragmentPin->PinType.PinSubCategoryObject = FragmentStruct;

		// 2. 创建临时变量（LocalWildcard），手动设置类型为FragmentStruct
		UK2Node_TemporaryVariable* TempVar = SpawnTempVarNode(
			UEdGraphSchema_K2::PC_Struct,
			NAME_None,
			FragmentStruct
		);

		// 3. 创建Assignment节点，将GetFragment的返回值赋值到临时变量
		UK2Node_AssignmentStatement* AssignNode = SpawnAssignNode(TempValuePin(TempVar));

		// 连接执行流: Exec → Assignment
		Link(ProxyExecPin(), ExecPin(AssignNode));

		// 连接: GetFragment.OutFragment → Assignment.Value
		Link(OutFragmentPin, AssignValuePin(AssignNode));

		// 4. 创建SetStructRecursiveMemberField节点
		UK2Node_SetStructRecursiveMember* MemberNode = SpawnNode<UK2Node_SetStructRecursiveMember>();
		MemberNode->StructMemberReference.StructType = FragmentStruct;
		MemberNode->StructMemberReference.MemberPaths = OwnerNode->StructMemberReference.MemberPaths;
		MemberNode->AllocateDefaultPins();
		MemberNode->OnMemberReferenceChanged();  // 创建输入引脚

		// 连接执行流: Assignment → MemberNode
		Link(ThenPin(AssignNode), ExecPin(MemberNode));

		// 连接: TempVar → MemberNode.Struct（操作临时变量）
		Link(TempValuePin(TempVar), NodePin(MemberNode, TEXT("Struct")));

		// 移动原始节点的成员输入引脚连接到MemberNode
		for (const FString& MemberPath : OwnerNode->StructMemberReference.MemberPaths)
		{
			FName PinName = UK2Node_SetMassFragment::InputPinName(MemberPath);
			UEdGraphPin* OriginalInputPin = ProxyPin(PinName);
			UEdGraphPin* MemberInputPin = NodePin(MemberNode, PinName.ToString());

			if (OriginalInputPin && MemberInputPin)
			{
				Link(OriginalInputPin, MemberInputPin);
			}
		}

		// 5. Create SetFragment function call
		UK2Node_CallFunction* SetterFunctionNode;

		switch (OwnerNode->CachedDataSourceType)
		{
		case EMassFragmentSourceDataType::EntityHandle:
			SetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFragment_Entity_Unified);
			Link(ProxyPin(TEXT("bDeferred")), FunctionInputPin(SetterFunctionNode, TEXT("bDeferred")));
			break;
		case EMassFragmentSourceDataType::EntityTemplateData:
			SetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFragment_Template_Unified);
			break;
		default:
			return;
		}

		// 连接执行流: MemberNode → SetterFunctionNode
		Link(ThenPin(MemberNode), ExecPin(SetterFunctionNode));

		// 从Knot节点输出连接到SetFragment
		Link(DataSourceKnot->GetOutputPin(), FunctionInputPin(SetterFunctionNode, SetterFunctionDataSourcePinName));
		Link(FragmentTypeKnot->GetOutputPin(), FunctionInputPin(SetterFunctionNode, TEXT("FragmentType")));

		// 手动设置SetFragment的InFragment引脚类型
		UEdGraphPin* InFragmentPin = FunctionInputPin(SetterFunctionNode, TEXT("InFragment"));
		InFragmentPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		InFragmentPin->PinType.PinSubCategoryObject = FragmentStruct;
		InFragmentPin->PinType.bIsReference = true;

		// 连接TempVar → SetFragment.InFragment（将修改后的临时变量写回）
		Link(TempValuePin(TempVar), InFragmentPin);

		// 连接最终的Then引脚
		Link(ThenPin(SetterFunctionNode), ProxyThenPin());
	}
	else
	{
		// 标准模式：直接设置整个Fragment

		UK2Node_CallFunction* SetterFunctionNode;
		FString SetterFunctionDataSourcePinName;
		switch (OwnerNode->CachedDataSourceType)
		{
		case EMassFragmentSourceDataType::EntityHandle:
			SetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFragment_Entity_Unified);
			SetterFunctionDataSourcePinName = TEXT("EntityHandle");
			Link(ProxyPin(TEXT("bDeferred")), FunctionInputPin(SetterFunctionNode, TEXT("bDeferred")));
			break;
		case EMassFragmentSourceDataType::EntityTemplateData:
			SetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFragment_Template_Unified);
			SetterFunctionDataSourcePinName = TEXT("TemplateData");
			break;
		default:
			return;
		}

		// 连接执行流
		Link(ProxyExecPin(), ExecPin(SetterFunctionNode));
		Link(ThenPin(SetterFunctionNode), ProxyThenPin());

		// 连接参数引脚
		Link(ProxyPin(UK2Node_SetMassFragment::DataSourcePinName()), FunctionInputPin(SetterFunctionNode, SetterFunctionDataSourcePinName));
		Link(ProxyPin(UK2Node_SetMassFragment::FragmentTypePinName()), FunctionInputPin(SetterFunctionNode, TEXT("FragmentType")));
		Link(ProxyPin(UK2Node_SetMassFragment::FragmentInPinName()), FunctionInputPin(SetterFunctionNode, TEXT("InFragment")));
	}
}

HNCH_EndExpandNode(UK2Node_SetMassFragment)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————