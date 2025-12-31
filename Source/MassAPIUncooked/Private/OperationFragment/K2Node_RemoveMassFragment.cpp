// Leroy Works & Ember, All Rights Reserved.

#include "OperationFragment/K2Node_RemoveMassFragment.h"
#include "MassAPIFuncLib.h"
#include "MassAPIStructs.h"
#include "MassEntityTypes.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "NodeCompiler/Magnus_HyperNodeCompilerHandler.h"
#include "K2Node_CallFunction.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace UK2Node_RemoveMassFragmentHelper
{
	// DataSource类型到标题的映射
	const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,				TEXT("RemoveMassFragment") },
		{ EMassFragmentSourceDataType::EntityHandle,		TEXT("RemoveMassFragment-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,	TEXT("RemoveMassFragment-Template") }
	};

	// 3. Clear - Icon Colors
	const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceIconColors =
	{
		{ EMassFragmentSourceDataType::None,                FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,        FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,  FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
	};

	// 3. Clear - Title Colors
	const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceTitleColors =
	{
		{ EMassFragmentSourceDataType::None,                FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,        FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,  FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
	};
}

using namespace UK2Node_RemoveMassFragmentHelper;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

static void SetDelegatePinType(UEdGraphPin* Pin)
{
	if (!Pin) return;

	// 1. Find the library function
	// NOTE: RemoveFragment is not a custom thunk but a regular UFunction in the library,
	// but SetFragment IS a custom thunk. The delegate signature is shared.
	// We can look up SetFragment to get the signature property easily.
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
				Pin->PinType.PinSubCategoryMemberReference.MemberParent = DelegateProp->SignatureFunction->GetOuter();
				Pin->PinType.PinSubCategoryMemberReference.MemberName = DelegateProp->SignatureFunction->GetFName();
			}
		}
	}
}

//================ Node.Configuration				========

//———————— Node.Appearance								————

FText UK2Node_RemoveMassFragment::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_RemoveMassFragment::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Composition"));
}

FText UK2Node_RemoveMassFragment::GetTooltipText() const
{
	return FText::FromString(TEXT("Remove a fragment from an entity or entity template."));
}

FSlateIcon UK2Node_RemoveMassFragment::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = DataSourceIconColors.FindRef(CachedDataSourceType);

	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UK2Node_RemoveMassFragment::GetNodeTitleColor() const
{
	return DataSourceTitleColors.FindRef(CachedDataSourceType);
}

//================ Pin.Management						========

//———————— Pin.Construction								————

void UK2Node_RemoveMassFragment::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsReference = true;

	UEdGraphPin* FragmentTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, UScriptStruct::StaticClass(), FragmentTypePinName());
	FragmentTypePin->PinType.PinSubCategory = UEdGraphSchema_K2::PSC_Self;

	// Create Deferred Pin
	UEdGraphPin* DeferredPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, DeferredPinName());
	DeferredPin->DefaultValue = TEXT("false");

	// Always create return value, visibility handled in Expand
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());

	Super::AllocateDefaultPins();
	UpdateDataSourceType();
}

//———————— Pin.Modification								————

void UK2Node_RemoveMassFragment::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
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

	// [FIX] Check OLD pins for the deferred value. 
	// The new pin (FindPin) will typically have the default "false" value at this stage,
	// so we must look at what the user had before reconstruction.
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

		// Apply the exact signature
		SetDelegatePinType(DelegatePin);

		// Restore Connections manually
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

	// 更新缓存的DataSource类型
	UpdateDataSourceType();

	// 恢复Fragment类型
	UScriptStruct* OldFragmentStruct = GetFragmentStructFromOldPins(OldPins);
	if (OldFragmentStruct)
	{
		if (UEdGraphPin* FragmentTypePin = FindPin(FragmentTypePinName()))
		{
			FragmentTypePin->DefaultObject = OldFragmentStruct;
		}
	}
}

void UK2Node_RemoveMassFragment::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
	OnFragmentTypeChanged();
}

//———————— Pin.ValueChange								————

void UK2Node_RemoveMassFragment::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

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

			// Apply exact signature immediately
			SetDelegatePinType(DelegatePin);

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
		}
	}
}

//———————— Pin.Connection								————

bool UK2Node_RemoveMassFragment::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
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

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_RemoveMassFragment::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
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

	// 当FragmentType引脚连接改变时，更新Fragment类型
	if (Pin && Pin->PinName == FragmentTypePinName())
	{
		if (Pin->LinkedTo.Num() == 0)
		{
			OnFragmentTypeChanged();
		}
	}
}

//================ DataSource.Access					========

void UK2Node_RemoveMassFragment::UpdateDataSourceType()
{
	// Detection Logic
	UEdGraphPin* DataSourcePin = FindPin(DataSourcePinName());
	if (!DataSourcePin) { CachedDataSourceType = EMassFragmentSourceDataType::None; return; }

	UScriptStruct* DataSourceStruct = Cast<UScriptStruct>(DataSourcePin->PinType.PinSubCategoryObject.Get());
	if (DataSourceStruct == FEntityHandle::StaticStruct()) CachedDataSourceType = EMassFragmentSourceDataType::EntityHandle;
	else if (DataSourceStruct == FEntityTemplateData::StaticStruct()) CachedDataSourceType = EMassFragmentSourceDataType::EntityTemplateData;
	else CachedDataSourceType = EMassFragmentSourceDataType::None;

	// Visibility Logic
	UEdGraphPin* DeferredPin = FindPin(DeferredPinName());
	if (DeferredPin)
	{
		const bool bShow = (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle);
		if (DeferredPin->bHidden == bShow)
		{
			DeferredPin->bHidden = !bShow;
			if (!bShow) DeferredPin->BreakAllPinLinks();
		}

		// Also handle OnFinished visibility if we are hiding Deferred
		UEdGraphPin* DelegatePin = FindPin(OnFinishedPinName());
		if (DelegatePin && !bShow)
		{
			DelegatePin->BreakAllPinLinks();
			RemovePin(DelegatePin);
		}
	}
}

//================ Fragment.Access						========

void UK2Node_RemoveMassFragment::OnFragmentTypeChanged()
{
	// RemoveMassFragment 不需要特殊处理，因为没有 InFragment 引脚
	// 只需要通知图表更新即可
	GetGraph()->NotifyNodeChanged(this);
}

bool UK2Node_RemoveMassFragment::IsValidFragmentStruct(const UScriptStruct* Struct) const
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

UScriptStruct* UK2Node_RemoveMassFragment::GetFragmentStruct() const
{
	UEdGraphPin* FragmentTypePin = FindPin(FragmentTypePinName());
	if (FragmentTypePin && FragmentTypePin->DefaultObject != nullptr && FragmentTypePin->LinkedTo.Num() == 0)
	{
		return Cast<UScriptStruct>(FragmentTypePin->DefaultObject);
	}
	return nullptr;
}

UScriptStruct* UK2Node_RemoveMassFragment::GetFragmentStructFromOldPins(const TArray<UEdGraphPin*>& OldPins) const
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

//================ Editor.PropertyChange				========

void UK2Node_RemoveMassFragment::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// 目前 RemoveMassFragment 没有需要特殊处理的属性
}

//================ Blueprint.Integration				========

void UK2Node_RemoveMassFragment::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

//================ Compiler.Integration					========

HNCH_StartExpandNode(UK2Node_RemoveMassFragment)

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

	// 根据DataSourceType选择对应的函数
	UK2Node_CallFunction* RemoveFunctionNode = nullptr;
	FString FunctionDataSourcePinName;

	switch (OwnerNode->CachedDataSourceType)
	{
	case EMassFragmentSourceDataType::EntityHandle:
		RemoveFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, RemoveFragment_Entity_Unified);
		FunctionDataSourcePinName = TEXT("EntityHandle");

		// Link bDeferred
		Link(ProxyPin(UK2Node_RemoveMassFragment::DeferredPinName()), FunctionInputPin(RemoveFunctionNode, TEXT("bDeferred")));

		// Connect OnFinished Delegate
		if (UEdGraphPin* DelegatePin = ProxyPin(UK2Node_RemoveMassFragment::OnFinishedPinName()))
		{
			Link(DelegatePin, FunctionInputPin(RemoveFunctionNode, TEXT("OnFinished")));
		}
		break;

	case EMassFragmentSourceDataType::EntityTemplateData:
		RemoveFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, RemoveFragment_Template_Unified);
		FunctionDataSourcePinName = TEXT("TemplateData");
		break;
	default:
		return;
	}

	// 连接执行流
	Link(ProxyExecPin(), ExecPin(RemoveFunctionNode));
	Link(ThenPin(RemoveFunctionNode), ProxyThenPin());

	// 连接参数引脚
	Link(ProxyPin(UK2Node_RemoveMassFragment::DataSourcePinName()), FunctionInputPin(RemoveFunctionNode, FunctionDataSourcePinName));
	Link(ProxyPin(UK2Node_RemoveMassFragment::FragmentTypePinName()), FunctionInputPin(RemoveFunctionNode, TEXT("FragmentType")));

	// 连接返回值引脚
	// 注意：RemoveFragment_Template 返回 void，所以只有 EntityHandle 版本才连接返回值
	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle)
	{
		UEdGraphPin* FunctionReturnPin = NodePin(RemoveFunctionNode, UEdGraphSchema_K2::PN_ReturnValue.ToString());
		if (FunctionReturnPin)
		{
			Link(FunctionReturnPin, ProxyPin(UK2Node_RemoveMassFragment::ReturnValuePinName()));
		}
	}
	else
	{
		// Template 版本没有返回值，隐藏 ReturnValue 引脚
		if (UEdGraphPin* ReturnValuePin = ProxyPin(UK2Node_RemoveMassFragment::ReturnValuePinName()))
		{
			ReturnValuePin->BreakAllPinLinks();
		}
	}
}

HNCH_EndExpandNode(UK2Node_RemoveMassFragment)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————