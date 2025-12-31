// Leroy Works & Ember, All Rights Reserved.

#include "MassAPIUncooked/Public/OperationTag/K2Node_AddMassTag.h"
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

namespace UK2Node_AddMassTagHelper
{
	// DataSource类型到标题的映射
	const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,				TEXT("AddMassTag") },
		{ EMassFragmentSourceDataType::EntityHandle,		TEXT("AddMassTag-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,	TEXT("AddMassTag-Template") }
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

using namespace UK2Node_AddMassTagHelper;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

static void SetDelegatePinType(UEdGraphPin* Pin)
{
	if (!Pin) return;

	// Use SetFragment_Entity_Unified from RemoveFragment logic as the reference signature
	// as it contains the definition for FOnMassDeferredFinished.
	UFunction* Function = UMassAPIFuncLib::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UMassAPIFuncLib, SetFragment_Entity_Unified));

	if (Function)
	{
		// Find the specific property "OnFinished"
		if (FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(Function->FindPropertyByName(TEXT("OnFinished"))))
		{
			Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Delegate;
			Pin->PinType.PinSubCategoryObject = DelegateProp->SignatureFunction;

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

FText UK2Node_AddMassTag::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_AddMassTag::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Tag"));
}

FText UK2Node_AddMassTag::GetTooltipText() const
{
	return FText::FromString(TEXT("Add a tag to an entity or entity template."));
}

FSlateIcon UK2Node_AddMassTag::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = DataSourceIconColors.FindRef(CachedDataSourceType);

	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UK2Node_AddMassTag::GetNodeTitleColor() const
{
	return DataSourceTitleColors.FindRef(CachedDataSourceType);
}

//================ Pin.Management						========

//———————— Pin.Construction								————

void UK2Node_AddMassTag::AllocateDefaultPins()
{
	// Create Exec/Then
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Create DataSource
	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsReference = true;

	// Create TagType
	UEdGraphPin* TagTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, UScriptStruct::StaticClass(), TagTypePinName());
	TagTypePin->PinType.PinSubCategory = UEdGraphSchema_K2::PSC_Self;

	// Create Deferred Pin (default false)
	UEdGraphPin* DeferredPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, DeferredPinName());
	DeferredPin->DefaultValue = TEXT("false");

	Super::AllocateDefaultPins();

	// Initialize
	UpdateDataSourceType();
}

//———————— Pin.Modification								————

void UK2Node_AddMassTag::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
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

	// Restore Deferred / Delegate Logic
	// [FIX] Check OLD pins for the deferred value.
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
		if (!DelegatePin)
		{
			DelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, OnFinishedPinName());
		}

		SetDelegatePinType(DelegatePin);

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

	// 恢复Tag类型
	UScriptStruct* OldTagStruct = GetTagStructFromOldPins(OldPins);
	if (OldTagStruct)
	{
		if (UEdGraphPin* TagTypePin = FindPin(TagTypePinName()))
		{
			TagTypePin->DefaultObject = OldTagStruct;
		}
	}
}

void UK2Node_AddMassTag::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
	OnTagTypeChanged();
}

//———————— Pin.ValueChange								————

void UK2Node_AddMassTag::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
	if (Pin && Pin->PinName == TagTypePinName())
	{
		OnTagTypeChanged();
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

bool UK2Node_AddMassTag::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
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

	// 限制TagType引脚只能连接有效的Tag结构体
	if (MyPin && MyPin->PinName == TagTypePinName())
	{
		if (const UScriptStruct* PinStruct = Cast<UScriptStruct>(OtherPin->PinType.PinSubCategoryObject.Get()))
		{
			if (!IsValidTagStruct(PinStruct))
			{
				OutReason = TEXT("The provided struct must be a valid Mass Tag type.");
				return true;
			}
		}
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_AddMassTag::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
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

	// 当TagType引脚连接改变时，更新Tag类型
	if (Pin && Pin->PinName == TagTypePinName())
	{
		if (Pin->LinkedTo.Num() == 0)
		{
			OnTagTypeChanged();
		}
	}
}

//================ DataSource.Access					========

void UK2Node_AddMassTag::UpdateDataSourceType()
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

	// Toggle bDeferred visibility based on source type
	UEdGraphPin* DeferredPin = FindPin(DeferredPinName());
	if (DeferredPin)
	{
		// Only Entities can handle deferred commands
		const bool bShow = (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle);

		if (DeferredPin->bHidden == bShow)
		{
			DeferredPin->bHidden = !bShow;
			if (!bShow)
			{
				DeferredPin->BreakAllPinLinks();
			}
		}

		// Also handle OnFinished visibility if we are hiding Deferred (e.g. switching to TemplateData mode)
		UEdGraphPin* DelegatePin = FindPin(OnFinishedPinName());
		if (DelegatePin && !bShow)
		{
			DelegatePin->BreakAllPinLinks();
			RemovePin(DelegatePin);
		}
	}
}

//================ Tag.Access							========

void UK2Node_AddMassTag::OnTagTypeChanged()
{
	// AddMassTag 不需要特殊处理，因为没有额外的引脚
	// 只需要通知图表更新即可
	GetGraph()->NotifyNodeChanged(this);
}

bool UK2Node_AddMassTag::IsValidTagStruct(const UScriptStruct* Struct) const
{
	if (!Struct)
	{
		return false;
	}

	// 检查是否是 FMassTag 派生类型
	return Struct->IsChildOf(FMassTag::StaticStruct());
}

UScriptStruct* UK2Node_AddMassTag::GetTagStruct() const
{
	UEdGraphPin* TagTypePin = FindPin(TagTypePinName());
	if (TagTypePin && TagTypePin->DefaultObject != nullptr && TagTypePin->LinkedTo.Num() == 0)
	{
		return Cast<UScriptStruct>(TagTypePin->DefaultObject);
	}
	return nullptr;
}

UScriptStruct* UK2Node_AddMassTag::GetTagStructFromOldPins(const TArray<UEdGraphPin*>& OldPins) const
{
	const UEdGraphPin* TagTypePin = nullptr;
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin->PinName == TagTypePinName())
		{
			TagTypePin = OldPin;
			break;
		}
	}

	if (TagTypePin && TagTypePin->DefaultObject != nullptr && TagTypePin->LinkedTo.Num() == 0)
	{
		return Cast<UScriptStruct>(TagTypePin->DefaultObject);
	}
	return nullptr;
}

//================ Editor.PropertyChange				========

void UK2Node_AddMassTag::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// 目前 AddMassTag 没有需要特殊处理的属性
}

//================ Blueprint.Integration				========

void UK2Node_AddMassTag::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

HNCH_StartExpandNode(UK2Node_AddMassTag)

virtual void Compile() override
{
	// 在编译前再次更新DataSourceType，确保类型正确
	OwnerNode->UpdateDataSourceType();

	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		CompilerContext.MessageLog.Error(TEXT("DataSource must be connected to either FEntityHandle or FEntityTemplateData. @@"), OwnerNode);
		return;
	}

	// 检查TagType是否为None
	UScriptStruct* TagStruct = OwnerNode->GetTagStruct();
	if (!TagStruct)
	{
		CompilerContext.MessageLog.Error(TEXT("TagType must be specified. Please select a valid Mass Tag type. @@"), OwnerNode);
		return;
	}

	// 根据DataSourceType选择对应的函数
	UK2Node_CallFunction* AddFunctionNode = nullptr;
	FString FunctionDataSourcePinName;

	switch (OwnerNode->CachedDataSourceType)
	{
	case EMassFragmentSourceDataType::EntityHandle:
		AddFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, AddTag_Entity);
		FunctionDataSourcePinName = TEXT("EntityHandle");

		// Link bDeferred
		Link(ProxyPin(UK2Node_AddMassTag::DeferredPinName()), FunctionInputPin(AddFunctionNode, TEXT("bDeferred")));

		// Connect OnFinished Delegate
		if (UEdGraphPin* DelegatePin = ProxyPin(UK2Node_AddMassTag::OnFinishedPinName()))
		{
			Link(DelegatePin, FunctionInputPin(AddFunctionNode, TEXT("OnFinished")));
		}
		break;

	case EMassFragmentSourceDataType::EntityTemplateData:
		AddFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, AddTag_Template);
		FunctionDataSourcePinName = TEXT("TemplateData");
		break;
	default:
		return;
	}

	// 连接执行流
	Link(ProxyExecPin(), ExecPin(AddFunctionNode));
	Link(ThenPin(AddFunctionNode), ProxyThenPin());

	// 连接参数引脚
	Link(ProxyPin(UK2Node_AddMassTag::DataSourcePinName()), FunctionInputPin(AddFunctionNode, FunctionDataSourcePinName));
	Link(ProxyPin(UK2Node_AddMassTag::TagTypePinName()), FunctionInputPin(AddFunctionNode, TEXT("TagType")));
}

HNCH_EndExpandNode(UK2Node_AddMassTag)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————