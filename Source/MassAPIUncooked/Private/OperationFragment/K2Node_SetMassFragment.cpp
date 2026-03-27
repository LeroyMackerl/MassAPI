/*
* MassAPI
* Created: 2025
* Author: Leroy Works, Ember, All Rights Reserved.
*/

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
#include "K2Node_ExecutionSequence.h"
#include "K2Node_IfThenElse.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/UnrealType.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace UK2Node_SetMembersInFragmentHelper
{
	// DataSource类型到标题的映射
	static const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,					TEXT("SetMassFragment") },
		{ EMassFragmentSourceDataType::EntityHandle,			TEXT("SetMassFragment-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,		TEXT("SetMassFragment-Template") },
		{ EMassFragmentSourceDataType::EntityHandleArray,		TEXT("SetMassFragment-Entities") },
		{ EMassFragmentSourceDataType::EntityTemplateDataArray,	TEXT("SetMassFragment-Templates") }
	};

	// 2. Set - Icon Colors
	static const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceIconColors =
	{
		{ EMassFragmentSourceDataType::None,                    FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,            FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,      FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandleArray,       FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateDataArray, FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
	};

	// 2. Set - Title Colors
	static const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceTitleColors =
	{
		{ EMassFragmentSourceDataType::None,                    FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,            FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,      FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandleArray,       FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateDataArray, FLinearColor(0.0f, 0.8f, 1.0f, 1.0f) },
	};

	// Helper to get the delegate signature from MassAPIFuncLib
	UFunction* GetOnMassDeferredFinishedSignature()
	{
		static UFunction* Signature = nullptr;
		if (!Signature)
		{
			// Find the function definition
			if (UFunction* Func = UMassAPIFuncLib::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UMassAPIFuncLib, SetFragment_Entity_Unified)))
			{
				// Extract the delegate signature from the 'OnFinished' property
				if (FDelegateProperty* Prop = CastField<FDelegateProperty>(Func->FindPropertyByName(TEXT("OnFinished"))))
				{
					Signature = Prop->SignatureFunction;
				}
			}
		}
		return Signature;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace K2Node_SetMassFragment_Local
{

static void SetDelegatePinType(UEdGraphPin* Pin)
{
	if (!Pin) { return; }

	// 1. Find the library function
	UFunction* Function = UMassAPIFuncLib::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UMassAPIFuncLib, SetFragment_Entity_Unified));

	if (Function)
	{
		// 2. Find the specific property "OnFinished"
		if (FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(Function->FindPropertyByName(TEXT("OnFinished"))))
		{
			// 3. Set the Category (Delegate)
			Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Delegate;

			// 4. Set the Signature Function (The "Type")
			Pin->PinType.PinSubCategoryObject = DelegateProp->SignatureFunction;

			// 5. [CRITICAL FIX] Set the Member Reference so the compiler knows the exact source
			if (DelegateProp->SignatureFunction)
			{
				FMemberReference::FillSimpleMemberReference(static_cast<UFunction*>(DelegateProp->SignatureFunction.Get()), Pin->PinType.PinSubCategoryMemberReference);
			}
		}
	}
}

} // namespace K2Node_SetMassFragment_Local

//================ Node.Configuration																			========

//———————— Node.Appearance																							————

FText UK2Node_SetMassFragment::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return  FText::FromString(UK2Node_SetMembersInFragmentHelper::DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_SetMassFragment::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Composition"));
}

FText UK2Node_SetMassFragment::GetTooltipText() const
{
	FString Tooltip = TEXT("Set members in a fragment.");

	if (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray ||
		CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateDataArray)
	{
		Tooltip += TEXT(" (Array mode: processes each element in a loop. For deferred mode, OnFinished fires once per element.)");
	}

	return FText::FromString(Tooltip);
}

FSlateIcon UK2Node_SetMassFragment::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = UK2Node_SetMembersInFragmentHelper::DataSourceIconColors.FindRef(CachedDataSourceType);

	static FSlateIcon Icon("EditorStyle", "GraphEditor.MakeStruct_16x");
	return Icon;
}

FLinearColor UK2Node_SetMassFragment::GetNodeTitleColor() const
{
	return UK2Node_SetMembersInFragmentHelper::DataSourceTitleColors.FindRef(CachedDataSourceType);
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

	// 2. Fragment Input Pin (Standard Mode only)
	if (!bSetMember)
	{
		UEdGraphPin* FragmentInPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, FragmentInPinName());
		FragmentInPin->PinType.bIsReference = false;  // 使用值传递，允许字面值输入
	}

	// 3. Deferred Pin
	UEdGraphPin* DeferredPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, DeferredPinName());
	DeferredPin->DefaultValue = TEXT("false");

	// 4. Delegate Pin (Dynamic)
	// In standard allocation, we don't usually create this unless we know bDeferred is true.
	// However, Super::AllocateDefaultPins() is just the start. 
	// Reconstruction/Value changes handle the visibility logic mostly.

	Super::AllocateDefaultPins();

	// 5. Initialize State
	UpdateDataSourceType();

	// 6. Create Member Pins (If in Set Member mode)
	if (bSetMember)
	{
		OnMemberReferenceChanged();
	}
}

//———————— Pin.Modification																							————

void UK2Node_SetMassFragment::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	// [FIX] Restore pin types BEFORE calling Super so it can properly restore connections.
	// Super::ReallocatePinsDuringReconstruction matches pins by name and restores connections,
	// but if the new pin is Wildcard and old pin was a struct type, it may fail/skip restoration.
	// By restoring types first, Super can properly handle connection restoration.

	// Pre-restore DataSource pin type
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin && OldPin->PinName == DataSourcePinName())
		{
			if (UEdGraphPin* NewDataSourcePin = FindPin(DataSourcePinName()))
			{
				NewDataSourcePin->PinType = OldPin->PinType;
				NewDataSourcePin->PinType.bIsReference = true;
			}
			break;
		}
	}

	// Pre-restore FragmentIn pin type (for non-bSetMember mode)
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin && OldPin->PinName == FragmentInPinName())
		{
			if (UEdGraphPin* NewFragmentInPin = FindPin(FragmentInPinName()))
			{
				NewFragmentInPin->PinType = OldPin->PinType;
				NewFragmentInPin->DefaultValue = OldPin->DefaultValue;
				NewFragmentInPin->AutogeneratedDefaultValue = OldPin->AutogeneratedDefaultValue;
			}
			break;
		}
	}

	Super::ReallocatePinsDuringReconstruction(OldPins);

	// 1. Restore DataSource Type (already done above, but keep this for UpdateDataSourceType cache update)
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin && OldPin->PinName == DataSourcePinName())
		{
			if (UEdGraphPin* NewDataSourcePin = FindPin(DataSourcePinName()))
			{
				NewDataSourcePin->PinType = OldPin->PinType;
				NewDataSourcePin->PinType.bIsReference = true;
			}
			break;
		}
	}

	// 2. Restore Deferred / Delegate Logic
	// [FIX] We must check OldPins for the deferred state, because the NEW pin
	// (created in AllocateDefaultPins) still has the default "false" value at this point.
	bool bWasDeferred = false;
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin && OldPin->PinName == DeferredPinName())
		{
			bWasDeferred = (OldPin->DefaultValue == TEXT("true"));
			break;
		}
	}

	if (bWasDeferred)
	{
		UEdGraphPin* DelegatePin = FindPin(OnFinishedPinName());

		// If it doesn't exist yet, create it
		if (!DelegatePin)
		{
			DelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, OnFinishedPinName());
		}

		// [FIX] Apply the exact signature
		K2Node_SetMassFragment_Local::SetDelegatePinType(DelegatePin);

		// [FIX] Restore Connections manually
		// Super::Reallocate... tries to match names, but if the pin didn't exist when Super ran,
		// connections might be lost. We check OldPins to recover them.
		for (UEdGraphPin* OldPin : OldPins)
		{
			if (OldPin && OldPin->PinName == OnFinishedPinName())
			{
				if (OldPin->LinkedTo.Num() > 0)
				{
					for (UEdGraphPin* Linked : OldPin->LinkedTo)
					{
						DelegatePin->MakeLinkTo(Linked);
					}
				}
				break;
			}
		}
	}

	// 3. Update Caches
	UpdateDataSourceType();

	// 4. Restore Fragment Type
	UScriptStruct* OldFragmentStruct = nullptr;
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

	if (!OldFragmentStruct)
	{
		OldFragmentStruct = GetFragmentStructFromOldPins(OldPins);
	}

	if (OldFragmentStruct)
	{
		if (UEdGraphPin* FragmentTypePin = FindPin(FragmentTypePinName()))
		{
			FragmentTypePin->DefaultObject = OldFragmentStruct;
		}
	}

	// 5. [FIX] Preserve member pin state (DefaultValues + Connections) for bSetMember mode
	// Store old pin state so OnMemberReferenceChanged can restore them
	if (bSetMember)
	{
		CachedOldPinStates.Empty();
		for (UEdGraphPin* OldPin : OldPins)
		{
			if (OldPin && OldPin->Direction == EGPD_Input &&
				OldPin->PinName != UEdGraphSchema_K2::PN_Execute &&
				OldPin->PinName != DataSourcePinName() &&
				OldPin->PinName != FragmentTypePinName() &&
				OldPin->PinName != DeferredPinName() &&
				OldPin->PinName != OnFinishedPinName())
			{
				// Store both DefaultValue and Connections for member pins
				if (!OldPin->DefaultValue.IsEmpty() || !OldPin->AutogeneratedDefaultValue.IsEmpty() || OldPin->LinkedTo.Num() > 0)
				{
					FCachedPinState& CachedState = CachedOldPinStates.Add(OldPin->PinName);
					CachedState.DefaultValue = OldPin->DefaultValue;
					CachedState.AutogeneratedDefaultValue = OldPin->AutogeneratedDefaultValue;
					CachedState.LinkedTo = OldPin->LinkedTo;
				}
			}
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

	// Handle Fragment Type Change
	if (Pin && Pin->PinName == FragmentTypePinName())
	{
		OnFragmentTypeChanged();
	}

	// Handle Deferred Checkbox Toggle
	if (Pin && Pin->PinName == DeferredPinName())
	{
		const bool bIsChecked = (Pin->DefaultValue == TEXT("true"));
		UEdGraphPin* DelegatePin = FindPin(OnFinishedPinName());

		bool bChanged = false;

		if (bIsChecked && !DelegatePin)
		{
			// User checked 'bDeferred': Create the pin
			DelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, OnFinishedPinName());

			// [FIX] Apply exact signature immediately
			K2Node_SetMassFragment_Local::SetDelegatePinType(DelegatePin);

			bChanged = true;
		}
		else if (!bIsChecked && DelegatePin)
		{
			// User unchecked 'bDeferred': Destroy the pin
			DelegatePin->BreakAllPinLinks();
			RemovePin(DelegatePin);
			bChanged = true;
		}

		if (bChanged)
		{
			GetGraph()->NotifyNodeChanged(this);
			// Typically refreshing the node visual is enough, strict structural mod not always needed for simple pin adds,
			// but safer to mark it if layout issues occur.
		}
	}
}

//———————— Pin.Connection																						————

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
	// 1. Get pin
	UEdGraphPin* DataSourcePin = FindPin(DataSourcePinName());
	if (!DataSourcePin)
	{
		CachedDataSourceType = EMassFragmentSourceDataType::None;
		return;
	}

	// 2. Extract struct and container types
	UScriptStruct* DataSourceStruct = Cast<UScriptStruct>(DataSourcePin->PinType.PinSubCategoryObject.Get());
	const bool bIsArray = (DataSourcePin->PinType.ContainerType == EPinContainerType::Array);

	// 3. Decision table
	if (DataSourceStruct == FEntityHandle::StaticStruct())
	{
		CachedDataSourceType = bIsArray ?
			EMassFragmentSourceDataType::EntityHandleArray :
			EMassFragmentSourceDataType::EntityHandle;
	}
	else if (DataSourceStruct == FEntityTemplateData::StaticStruct())
	{
		CachedDataSourceType = bIsArray ?
			EMassFragmentSourceDataType::EntityTemplateDataArray :
			EMassFragmentSourceDataType::EntityTemplateData;
	}
	else
	{
		CachedDataSourceType = EMassFragmentSourceDataType::None;
	}

	// 4. Visibility logic - bDeferred now supported for arrays too
	UEdGraphPin* DeferredPin = FindPin(TEXT("bDeferred"));
	if (DeferredPin)
	{
		const bool bShow = (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle ||
							CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray);

		if (DeferredPin->bHidden == bShow)
		{
			DeferredPin->bHidden = !bShow;
			if (!bShow) DeferredPin->BreakAllPinLinks();
		}

		UEdGraphPin* DelegatePin = FindPin(OnFinishedPinName());
		if (DelegatePin && !bShow)
		{
			DelegatePin->BreakAllPinLinks();
			RemovePin(DelegatePin);
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
				FragmentInPin->PinType.bIsReference = false;  // 使用值传递，允许字面值输入
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
		return;
	}

	UScriptStruct* FragmentStruct = GetFragmentStruct();

	// Sync struct type
	if (StructMemberReference.StructType != FragmentStruct)
	{
		StructMemberReference.StructType = FragmentStruct;
	}

	// --- 1. Preserve Connections & State ---
	TMap<FName, FCachedPinState> OldMemberPinStates;  // [FIX] Store both connections and values
	TArray<UEdGraphPin*> OldDeferredPinConnections;
	TArray<UEdGraphPin*> OldDelegatePinConnections;

	bool bWasDeferredPinVisible = true;
	FString SavedDeferredDefaultValue = TEXT("false");
	bool bWasDelegatePinPresent = false;

	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			if (Pin->PinName == DeferredPinName())
			{
				OldDeferredPinConnections = Pin->LinkedTo;
				bWasDeferredPinVisible = !Pin->bHidden;
				SavedDeferredDefaultValue = Pin->DefaultValue; // Save the value (true/false)
			}
			else if (Pin->PinName == OnFinishedPinName())
			{
				OldDelegatePinConnections = Pin->LinkedTo;
				bWasDelegatePinPresent = true;
			}
			else if (Pin->PinName != UEdGraphSchema_K2::PN_Execute &&
				Pin->PinName != DataSourcePinName() &&
				Pin->PinName != FragmentTypePinName())
			{
				// [FIX] Preserve both connections and DefaultValues
				if (Pin->LinkedTo.Num() > 0 || !Pin->DefaultValue.IsEmpty() || !Pin->AutogeneratedDefaultValue.IsEmpty())
				{
					FCachedPinState& CachedState = OldMemberPinStates.Add(Pin->PinName);
					CachedState.DefaultValue = Pin->DefaultValue;
					CachedState.AutogeneratedDefaultValue = Pin->AutogeneratedDefaultValue;
					CachedState.LinkedTo = Pin->LinkedTo;
				}
			}
		}
	}

	// [FIX] Merge with any cached state from ReallocatePinsDuringReconstruction
	// (these take priority as they come from the actual old pins before destruction)
	for (const auto& CachedPair : CachedOldPinStates)
	{
		if (!OldMemberPinStates.Contains(CachedPair.Key))
		{
			OldMemberPinStates.Add(CachedPair.Key, CachedPair.Value);
		}
	}
	CachedOldPinStates.Empty();  // Clear after use

	// --- 2. Remove Dynamic Input Pins ---
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
	if (StructMemberReference.IsValid() && FragmentStruct)
	{
		// ... [Standard Member Pin Creation Logic - Same as your original code] ...
		TArray<TPair<FString, TArray<int32>>> PathsWithOffsets;
		for (const FString& MemberPath : StructMemberReference.MemberPaths)
		{
			TArray<int32> OffsetIndices = StructMemberHelper::GetMemberOffsetIndices(FragmentStruct, MemberPath);
			if (OffsetIndices.Num() > 0) PathsWithOffsets.Add(TPair<FString, TArray<int32>>(MemberPath, OffsetIndices));
		}

		PathsWithOffsets.Sort([](const TPair<FString, TArray<int32>>& A, const TPair<FString, TArray<int32>>& B) {
			return StructMemberHelper::CompareOffsetIndices(A.Value, B.Value);
			});

		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		for (const auto& Pair : PathsWithOffsets)
		{
			const FString& MemberPath = Pair.Key;
			FName PinName = InputPinName(MemberPath);
			FProperty* Property = StructMemberHelper::FindPropertyByRecursivePath(FragmentStruct, MemberPath);

			if (!Property || !StructMemberHelper::CanCreatePinForProperty(Property)) continue;

			FEdGraphPinType NewPinType;
			if (Schema->ConvertPropertyToPinType(Property, NewPinType))
			{
				UEdGraphPin* NewPin = CreatePin(EGPD_Input, NewPinType.PinCategory, PinName);
				NewPin->PinType = NewPinType;
				NewPin->PinType.bIsReference = false;  // 使用值传递，允许字面值输入
				NewPin->PinFriendlyName = FText::FromString(MemberPath);
				bPinTypeChanged = true;

				// [FIX] Restore both connections and DefaultValue from cached state
				if (FCachedPinState* CachedState = OldMemberPinStates.Find(PinName))
				{
					// Restore connections
					for (UEdGraphPin* LinkedPin : CachedState->LinkedTo)
					{
						if (LinkedPin)
						{
							NewPin->MakeLinkTo(LinkedPin);
						}
					}
					// Restore DefaultValue (literal values entered by user)
					NewPin->DefaultValue = CachedState->DefaultValue;
					NewPin->AutogeneratedDefaultValue = CachedState->AutogeneratedDefaultValue;
				}
			}
		}
	}

	// --- 4. Recreate Deferred Pin ---
	UEdGraphPin* NewDeferredPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, DeferredPinName());
	NewDeferredPin->DefaultValue = SavedDeferredDefaultValue; // Restore value (Critical!)
	NewDeferredPin->bHidden = !bWasDeferredPinVisible;

	for (UEdGraphPin* LinkedPin : OldDeferredPinConnections)
	{
		if (LinkedPin) NewDeferredPin->MakeLinkTo(LinkedPin);
	}

	// --- 5. Recreate Delegate Pin (If deferred was checked) ---
	// [FIX] Check the saved value. If "true", we must create the delegate pin.
	if (SavedDeferredDefaultValue == TEXT("true"))
	{
		UEdGraphPin* NewDelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, OnFinishedPinName());

		// [FIX] Apply exact signature
		K2Node_SetMassFragment_Local::SetDelegatePinType(NewDelegatePin);

		for (UEdGraphPin* LinkedPin : OldDelegatePinConnections)
		{
			if (LinkedPin) NewDelegatePin->MakeLinkTo(LinkedPin);
		}
	}

	// --- 6. Finalize ---
	UpdateDataSourceType();

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

	// Validate array mode
	bool bIsArray = (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray ||
					 OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateDataArray);

	if (bIsArray)
	{
		UEdGraphPin* DataSourcePin = ProxyPin(UK2Node_SetMassFragment::DataSourcePinName());
		if (!DataSourcePin || DataSourcePin->LinkedTo.Num() == 0)
		{
			CompilerContext.MessageLog.Error(TEXT("DataSource array must be connected. @@"), OwnerNode);
			return;
		}

		if (DataSourcePin->PinType.ContainerType != EPinContainerType::Array)
		{
			CompilerContext.MessageLog.Error(TEXT("Internal error: DataSource type mismatch (expected array). @@"), OwnerNode);
			return;
		}
	}

	// Array mode handling
	if (bIsArray)
	{
		// ARRAY PATH: Generate ForEach loop
		bool bIsEntityMode = (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandleArray);

		// 1. Create loop counter
		UK2Node_TemporaryVariable* LoopCounterNode = SpawnNode<UK2Node_TemporaryVariable>();
		LoopCounterNode->VariableType.PinCategory = UEdGraphSchema_K2::PC_Int;
		LoopCounterNode->AllocateDefaultPins();
		UEdGraphPin* LoopCounterPin = LoopCounterNode->GetVariablePin();

		// 2. Array Length
		UK2Node_CallFunction* ArrayLengthNode = HNCH_SpawnFunctionNode(UKismetArrayLibrary, Array_Length);
		UEdGraphPin* LengthArrayPin = ArrayLengthNode->FindPinChecked(TEXT("TargetArray"), EGPD_Input);
		LengthArrayPin->PinType = ProxyPin(UK2Node_SetMassFragment::DataSourcePinName())->PinType;
		CompilerContext.CopyPinLinksToIntermediate(*ProxyPin(UK2Node_SetMassFragment::DataSourcePinName()), *LengthArrayPin);

		// 3. Counter initialization
		UK2Node_AssignmentStatement* CounterInitNode = SpawnNode<UK2Node_AssignmentStatement>();
		CounterInitNode->AllocateDefaultPins();
		Link(LoopCounterPin, CounterInitNode->GetVariablePin());
		CounterInitNode->GetValuePin()->DefaultValue = TEXT("0");

		// 4. Branch node
		UK2Node_IfThenElse* BranchNode = SpawnNode<UK2Node_IfThenElse>();
		BranchNode->AllocateDefaultPins();

		// 5. Loop condition (counter < length)
		UK2Node_CallFunction* ConditionNode = HNCH_SpawnFunctionNode(UKismetMathLibrary, Less_IntInt);
		Link(LoopCounterPin, ConditionNode->FindPinChecked(TEXT("A")));
		Link(ArrayLengthNode->GetReturnValuePin(), ConditionNode->FindPinChecked(TEXT("B")));
		Link(ConditionNode->GetReturnValuePin(), BranchNode->GetConditionPin());

		// 6. Execution sequence
		UK2Node_ExecutionSequence* SequenceNode = SpawnNode<UK2Node_ExecutionSequence>();
		SequenceNode->AllocateDefaultPins();

		// 7. Array Get (extract element)
		UK2Node_CallFunction* ArrayGetNode = HNCH_SpawnFunctionNode(UKismetArrayLibrary, Array_Get);
		UEdGraphPin* GetArrayPin = ArrayGetNode->FindPinChecked(TEXT("TargetArray"), EGPD_Input);
		GetArrayPin->PinType = ProxyPin(UK2Node_SetMassFragment::DataSourcePinName())->PinType;
		CompilerContext.CopyPinLinksToIntermediate(*ProxyPin(UK2Node_SetMassFragment::DataSourcePinName()), *GetArrayPin);
		Link(LoopCounterPin, ArrayGetNode->FindPinChecked(TEXT("Index")));

		// Set array element output pin type (copy from array but change container to None)
		UEdGraphPin* ArrayItemPin = ArrayGetNode->FindPinChecked(TEXT("Item"));
		ArrayItemPin->PinType = ProxyPin(UK2Node_SetMassFragment::DataSourcePinName())->PinType;
		ArrayItemPin->PinType.ContainerType = EPinContainerType::None;

		// 8. Check if bSetMember mode for arrays
		if (OwnerNode->bSetMember)
		{
			// bSetMember mode with arrays: GetFragment → SetStructRecursiveMember → SetFragment for each element

			// Validate MemberPaths
			if (!OwnerNode->StructMemberReference.IsValid())
			{
				CompilerContext.MessageLog.Error(TEXT("No members selected. Please select members in the Details panel. @@"), OwnerNode);
				return;
			}

			// For template mode, we need a temp var for the template element since Array_Get returns a copy
			UK2Node_TemporaryVariable* TempTemplateVar = nullptr;
			UEdGraphPin* TemplateSourcePin = nullptr;
			UK2Node_AssignmentStatement* TempTemplateAssignNode = nullptr;

			if (!bIsEntityMode)
			{
				// Create temporary variable to hold the template element
				TempTemplateVar = SpawnNode<UK2Node_TemporaryVariable>();
				TempTemplateVar->VariableType = ArrayItemPin->PinType;
				TempTemplateVar->AllocateDefaultPins();
				TemplateSourcePin = TempTemplateVar->GetVariablePin();

				// Assign array element to temp var
				TempTemplateAssignNode = SpawnNode<UK2Node_AssignmentStatement>();
				TempTemplateAssignNode->AllocateDefaultPins();
				Link(TemplateSourcePin, TempTemplateAssignNode->GetVariablePin());
				Link(ArrayItemPin, TempTemplateAssignNode->GetValuePin());
			}

			// Create Knot nodes for distribution (array element/temp var and FragmentType need to go to multiple places)
			UK2Node_Knot* ArrayElementKnot = SpawnKnotNode();
			ArrayElementKnot->GetInputPin()->PinType = ArrayItemPin->PinType;
			if (bIsEntityMode)
			{
				Link(ArrayItemPin, ArrayElementKnot->GetInputPin());
			}
			else
			{
				Link(TemplateSourcePin, ArrayElementKnot->GetInputPin());
			}

			UK2Node_Knot* FragmentTypeKnot = SpawnKnotNode();
			Link(ProxyPin(UK2Node_SetMassFragment::FragmentTypePinName()), FragmentTypeKnot->GetInputPin());

			// Create GetFragment function call
			UK2Node_CallFunction* GetterFunctionNode;
			FString GetterFunctionDataSourcePinName;
			FString SetterFunctionDataSourcePinName;

			if (bIsEntityMode)
			{
				GetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, GetFragment_Entity_Unified);
				GetterFunctionDataSourcePinName = TEXT("EntityHandle");
				SetterFunctionDataSourcePinName = TEXT("EntityHandle");
			}
			else
			{
				GetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, GetFragment_Template_Unified);
				GetterFunctionDataSourcePinName = TEXT("TemplateData");
				SetterFunctionDataSourcePinName = TEXT("TemplateData");
			}

			// Connect array element knot to GetFragment
			Link(ArrayElementKnot->GetOutputPin(), FunctionInputPin(GetterFunctionNode, GetterFunctionDataSourcePinName));
			Link(FragmentTypeKnot->GetOutputPin(), FunctionInputPin(GetterFunctionNode, TEXT("FragmentType")));

			// Get OutFragment pin and set type
			UEdGraphPin* OutFragmentPin = NodePin(GetterFunctionNode, TEXT("OutFragment"));
			OutFragmentPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			OutFragmentPin->PinType.PinSubCategoryObject = FragmentStruct;

			// Create temporary variable
			UK2Node_TemporaryVariable* TempVar = SpawnTempVarNode(
				UEdGraphSchema_K2::PC_Struct,
				NAME_None,
				FragmentStruct
			);

			// Create Assignment node
			UK2Node_AssignmentStatement* AssignNode = SpawnAssignNode(TempValuePin(TempVar));
			Link(OutFragmentPin, AssignValuePin(AssignNode));

			// Create SetStructRecursiveMember node
			UK2Node_SetStructRecursiveMember* MemberNode = SpawnNode<UK2Node_SetStructRecursiveMember>();
			MemberNode->StructMemberReference.StructType = FragmentStruct;
			MemberNode->StructMemberReference.MemberPaths = OwnerNode->StructMemberReference.MemberPaths;
			MemberNode->AllocateDefaultPins();
			MemberNode->OnMemberReferenceChanged();

			// Connect TempVar to MemberNode
			Link(TempValuePin(TempVar), NodePin(MemberNode, TEXT("Struct")));

			// Move member input pins
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

			// Create SetFragment function call
			UK2Node_CallFunction* SetterFunctionNode;

			if (bIsEntityMode)
			{
				SetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFragment_Entity_Unified);
				Link(ProxyPin(TEXT("bDeferred")), FunctionInputPin(SetterFunctionNode, TEXT("bDeferred")));
				if (UEdGraphPin* DelegatePin = ProxyPin(UK2Node_SetMassFragment::OnFinishedPinName()))
				{
					Link(DelegatePin, FunctionInputPin(SetterFunctionNode, TEXT("OnFinished")));
				}
			}
			else
			{
				SetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFragment_Template_Unified);
			}

			// Connect array element knot to SetFragment
			Link(ArrayElementKnot->GetOutputPin(), FunctionInputPin(SetterFunctionNode, SetterFunctionDataSourcePinName));
			Link(FragmentTypeKnot->GetOutputPin(), FunctionInputPin(SetterFunctionNode, TEXT("FragmentType")));

			// Set InFragment pin type and connect TempVar
			UEdGraphPin* InFragmentPin = FunctionInputPin(SetterFunctionNode, TEXT("InFragment"));
			InFragmentPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			InFragmentPin->PinType.PinSubCategoryObject = FragmentStruct;
			InFragmentPin->PinType.bIsReference = true;
			Link(TempValuePin(TempVar), InFragmentPin);

			// Connect execution flow
			if (bIsEntityMode)
			{
				Link(SequenceNode->GetThenPinGivenIndex(0), ExecPin(AssignNode));
				Link(ThenPin(AssignNode), ExecPin(MemberNode));
				Link(ThenPin(MemberNode), ExecPin(SetterFunctionNode));
				Link(ThenPin(SetterFunctionNode), SequenceNode->GetThenPinGivenIndex(1));
			}
			else
			{
				// Template mode: need Array_Set to write modified template back
				UK2Node_CallFunction* ArraySetNode = HNCH_SpawnFunctionNode(UKismetArrayLibrary, Array_Set);
				UEdGraphPin* SetArrayPin = ArraySetNode->FindPinChecked(TEXT("TargetArray"), EGPD_Input);
				SetArrayPin->PinType = ProxyPin(UK2Node_SetMassFragment::DataSourcePinName())->PinType;
				CompilerContext.CopyPinLinksToIntermediate(*ProxyPin(UK2Node_SetMassFragment::DataSourcePinName()), *SetArrayPin);
				Link(LoopCounterPin, ArraySetNode->FindPinChecked(TEXT("Index")));
				UEdGraphPin* SetItemPin = ArraySetNode->FindPinChecked(TEXT("Item"));
				SetItemPin->PinType = TemplateSourcePin->PinType;
				Link(TemplateSourcePin, SetItemPin);

				// Execution: TempTemplateAssign → AssignFragment → MemberNode → SetFragment → Array_Set
				Link(SequenceNode->GetThenPinGivenIndex(0), ExecPin(TempTemplateAssignNode));
				Link(ThenPin(TempTemplateAssignNode), ExecPin(AssignNode));
				Link(ThenPin(AssignNode), ExecPin(MemberNode));
				Link(ThenPin(MemberNode), ExecPin(SetterFunctionNode));
				Link(ThenPin(SetterFunctionNode), ExecPin(ArraySetNode));
				Link(ThenPin(ArraySetNode), SequenceNode->GetThenPinGivenIndex(1));

				// Reconstruct Array_Set node to finalize pin types
				ArraySetNode->PostReconstructNode();
			}
		}
		else
		{
			// Standard mode with arrays: Direct SetFragment call for each element

			// Create SetFragment function call
			UK2Node_CallFunction* SetterFunctionNode;

			if (bIsEntityMode)
			{
				SetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFragment_Entity_Unified);
				Link(ArrayItemPin, FunctionInputPin(SetterFunctionNode, TEXT("EntityHandle")));
				Link(ProxyPin(TEXT("bDeferred")), FunctionInputPin(SetterFunctionNode, TEXT("bDeferred")));
				if (UEdGraphPin* DelegatePin = ProxyPin(UK2Node_SetMassFragment::OnFinishedPinName()))
				{
					Link(DelegatePin, FunctionInputPin(SetterFunctionNode, TEXT("OnFinished")));
				}

				Link(ProxyPin(UK2Node_SetMassFragment::FragmentTypePinName()), FunctionInputPin(SetterFunctionNode, TEXT("FragmentType")));
				Link(ProxyPin(UK2Node_SetMassFragment::FragmentInPinName()), FunctionInputPin(SetterFunctionNode, TEXT("InFragment")));

				// Connect execution flow
				Link(SequenceNode->GetThenPinGivenIndex(0), ExecPin(SetterFunctionNode));
				Link(ThenPin(SetterFunctionNode), SequenceNode->GetThenPinGivenIndex(1));
			}
			else
			{
				// Template mode: Array_Get returns a copy, so we need to use a temp var and Array_Set to write back

				// Create temporary variable to hold the template element
				UK2Node_TemporaryVariable* TempTemplateVar = SpawnNode<UK2Node_TemporaryVariable>();
				TempTemplateVar->VariableType = ArrayItemPin->PinType;
				TempTemplateVar->AllocateDefaultPins();
				UEdGraphPin* TempTemplatePin = TempTemplateVar->GetVariablePin();

				// Assign array element to temp var
				UK2Node_AssignmentStatement* TempAssignNode = SpawnNode<UK2Node_AssignmentStatement>();
				TempAssignNode->AllocateDefaultPins();
				Link(TempTemplatePin, TempAssignNode->GetVariablePin());
				Link(ArrayItemPin, TempAssignNode->GetValuePin());

				// Call SetFragment_Template_Unified with the temp var (by ref, so it gets modified)
				SetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFragment_Template_Unified);
				Link(TempTemplatePin, FunctionInputPin(SetterFunctionNode, TEXT("TemplateData")));
				Link(ProxyPin(UK2Node_SetMassFragment::FragmentTypePinName()), FunctionInputPin(SetterFunctionNode, TEXT("FragmentType")));
				Link(ProxyPin(UK2Node_SetMassFragment::FragmentInPinName()), FunctionInputPin(SetterFunctionNode, TEXT("InFragment")));

				// Array_Set to write the modified template back into the array
				UK2Node_CallFunction* ArraySetNode = HNCH_SpawnFunctionNode(UKismetArrayLibrary, Array_Set);
				UEdGraphPin* SetArrayPin = ArraySetNode->FindPinChecked(TEXT("TargetArray"), EGPD_Input);
				SetArrayPin->PinType = ProxyPin(UK2Node_SetMassFragment::DataSourcePinName())->PinType;
				CompilerContext.CopyPinLinksToIntermediate(*ProxyPin(UK2Node_SetMassFragment::DataSourcePinName()), *SetArrayPin);
				Link(LoopCounterPin, ArraySetNode->FindPinChecked(TEXT("Index")));
				UEdGraphPin* SetItemPin = ArraySetNode->FindPinChecked(TEXT("Item"));
				SetItemPin->PinType = TempTemplatePin->PinType;
				Link(TempTemplatePin, SetItemPin);

				// Connect execution flow: TempAssign → SetFragment → Array_Set
				Link(SequenceNode->GetThenPinGivenIndex(0), ExecPin(TempAssignNode));
				Link(ThenPin(TempAssignNode), ExecPin(SetterFunctionNode));
				Link(ThenPin(SetterFunctionNode), ExecPin(ArraySetNode));
				Link(ThenPin(ArraySetNode), SequenceNode->GetThenPinGivenIndex(1));

				// Reconstruct Array_Set node to finalize pin types
				ArraySetNode->PostReconstructNode();
			}
		}

		// 9. Increment counter
		UK2Node_CallFunction* IncrementNode = HNCH_SpawnFunctionNode(UKismetMathLibrary, Add_IntInt);
		Link(LoopCounterPin, IncrementNode->FindPinChecked(TEXT("A")));
		IncrementNode->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("1");

		UK2Node_AssignmentStatement* CounterAssignNode = SpawnNode<UK2Node_AssignmentStatement>();
		CounterAssignNode->AllocateDefaultPins();
		Link(LoopCounterPin, CounterAssignNode->GetVariablePin());
		Link(IncrementNode->GetReturnValuePin(), CounterAssignNode->GetValuePin());

		// 10. Connect execution flow
		Link(ProxyExecPin(), ExecPin(CounterInitNode));
		Link(ThenPin(CounterInitNode), ExecPin(BranchNode));
		Link(BranchNode->GetThenPin(), ExecPin(SequenceNode));
		Link(SequenceNode->GetThenPinGivenIndex(1), ExecPin(CounterAssignNode));
		Link(ThenPin(CounterAssignNode), ExecPin(BranchNode));  // Loop back
		Link(BranchNode->GetElsePin(), ProxyThenPin());

		// 11. Reconstruct array function nodes to finalize pin types (must be after all connections)
		ArrayLengthNode->PostReconstructNode();
		ArrayGetNode->PostReconstructNode();
	}
	// 根据bSetMember决定编译方式 (SINGLE-ENTITY PATH)
	else if (OwnerNode->bSetMember)
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
				if (OriginalInputPin->LinkedTo.Num() > 0)
				{
					// 有连接，传递连接
					Link(OriginalInputPin, MemberInputPin);
				}
				else if (!OriginalInputPin->DefaultValue.IsEmpty())
				{
					// 处理字面值：直接复制 DefaultValue
					MemberInputPin->DefaultValue = OriginalInputPin->DefaultValue;
					MemberInputPin->bDefaultValueIsIgnored = false;
					MemberInputPin->bDefaultValueIsReadOnly = OriginalInputPin->bDefaultValueIsReadOnly;
				}
			}
		}

		// 5. Create SetFragment function call
		UK2Node_CallFunction* SetterFunctionNode;

		switch (OwnerNode->CachedDataSourceType)
		{
		case EMassFragmentSourceDataType::EntityHandle:
			SetterFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, SetFragment_Entity_Unified);
			Link(ProxyPin(TEXT("bDeferred")), FunctionInputPin(SetterFunctionNode, TEXT("bDeferred")));
			// Connect OnFinished Delegate
			if (UEdGraphPin* DelegatePin = ProxyPin(UK2Node_SetMassFragment::OnFinishedPinName()))
			{
				Link(DelegatePin, FunctionInputPin(SetterFunctionNode, TEXT("OnFinished")));
			}
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
			// Connect OnFinished Delegate
			if (UEdGraphPin* DelegatePin = ProxyPin(UK2Node_SetMassFragment::OnFinishedPinName()))
			{
				Link(DelegatePin, FunctionInputPin(SetterFunctionNode, TEXT("OnFinished")));
			}
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

		// 处理 FragmentIn 引脚：支持连接和字面值
		UEdGraphPin* FragmentInPin = ProxyPin(UK2Node_SetMassFragment::FragmentInPinName());
		UEdGraphPin* TargetInFragmentPin = FunctionInputPin(SetterFunctionNode, TEXT("InFragment"));
		if (FragmentInPin->LinkedTo.Num() > 0)
		{
			Link(FragmentInPin, TargetInFragmentPin);
		}
		else if (!FragmentInPin->DefaultValue.IsEmpty())
		{
			// 处理字面值
			TargetInFragmentPin->DefaultValue = FragmentInPin->DefaultValue;
			TargetInFragmentPin->bDefaultValueIsIgnored = false;
			TargetInFragmentPin->bDefaultValueIsReadOnly = FragmentInPin->bDefaultValueIsReadOnly;
		}
	}
}

HNCH_EndExpandNode(UK2Node_SetMassFragment)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————