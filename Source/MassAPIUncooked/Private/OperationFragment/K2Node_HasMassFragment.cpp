// Leroy Works & Ember, All Rights Reserved.

#include "OperationFragment/K2Node_HasMassFragment.h"
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

namespace UK2Node_HasMassFragmentHelper
{
	// DataSource类型到标题的映射
	const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,				TEXT("HasMassFragment") },
		{ EMassFragmentSourceDataType::EntityHandle,		TEXT("HasMassFragment-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,	TEXT("HasMassFragment-Template") }
	};

	// 1. Has - Icon Colors
	const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceIconColors =
	{
		{ EMassFragmentSourceDataType::None,                FLinearColor(1.0f, 0.9f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,        FLinearColor(1.0f, 0.9f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,  FLinearColor(1.0f, 0.9f, 0.0f, 1.0f) },
	};

	// 1. Has - Title Colors
	const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceTitleColors =
	{
		{ EMassFragmentSourceDataType::None,                FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,        FLinearColor(1.0f, 0.9f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,  FLinearColor(1.0f, 0.9f, 0.0f, 1.0f) },
	};
}

using namespace UK2Node_HasMassFragmentHelper;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration				========

//———————— Node.Appearance								————

FText UK2Node_HasMassFragment::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_HasMassFragment::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Composition"));
}

FText UK2Node_HasMassFragment::GetTooltipText() const
{
	return FText::FromString(TEXT("Check if an entity or entity template has a fragment."));
}

FSlateIcon UK2Node_HasMassFragment::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = DataSourceIconColors.FindRef(CachedDataSourceType);

	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UK2Node_HasMassFragment::GetNodeTitleColor() const
{
	return DataSourceTitleColors.FindRef(CachedDataSourceType);
}

//================ Pin.Management						========

//———————— Pin.Construction								————

void UK2Node_HasMassFragment::AllocateDefaultPins()
{
	// 纯节点，不需要执行引脚

	// 创建DataSource引脚（Wildcard，支持FEntityHandle和FEntityTemplateData）
	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsConst = true;

	// 创建FragmentType引脚（用于类型选择）
	UEdGraphPin* FragmentTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, UScriptStruct::StaticClass(), FragmentTypePinName());
	FragmentTypePin->PinType.PinSubCategory = UEdGraphSchema_K2::PSC_Self;

	// 创建返回值引脚（bool类型）
	UEdGraphPin* ReturnValuePin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());

	Super::AllocateDefaultPins();

	// 初始化DataSource类型
	UpdateDataSourceType();
}

//———————— Pin.Modification								————

void UK2Node_HasMassFragment::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
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
				NewDataSourcePin->PinType.bIsConst = true; // 确保const标志被保留
			}
			break;
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

void UK2Node_HasMassFragment::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
	OnFragmentTypeChanged();
}

//———————— Pin.ValueChange								————

void UK2Node_HasMassFragment::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
	if (Pin && Pin->PinName == FragmentTypePinName())
	{
		OnFragmentTypeChanged();
	}
}

//———————— Pin.Connection								————

bool UK2Node_HasMassFragment::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
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

void UK2Node_HasMassFragment::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
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
					DataSourcePin->PinType.bIsConst = true; // 确保const标志被保留
				}
			}
			else
			{
				// 断开连接，恢复为Wildcard
				DataSourcePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
				DataSourcePin->PinType.PinSubCategoryObject = nullptr;
				DataSourcePin->PinType.bIsConst = true; // 确保const标志被保留
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

void UK2Node_HasMassFragment::UpdateDataSourceType()
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

//================ Fragment.Access						========

void UK2Node_HasMassFragment::OnFragmentTypeChanged()
{
	// HasMassFragment 不需要特殊处理，因为是纯查询节点
	// 只需要通知图表更新即可
	GetGraph()->NotifyNodeChanged(this);
}

bool UK2Node_HasMassFragment::IsValidFragmentStruct(const UScriptStruct* Struct) const
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

UScriptStruct* UK2Node_HasMassFragment::GetFragmentStruct() const
{
	UEdGraphPin* FragmentTypePin = FindPin(FragmentTypePinName());
	if (FragmentTypePin && FragmentTypePin->DefaultObject != nullptr && FragmentTypePin->LinkedTo.Num() == 0)
	{
		return Cast<UScriptStruct>(FragmentTypePin->DefaultObject);
	}
	return nullptr;
}

UScriptStruct* UK2Node_HasMassFragment::GetFragmentStructFromOldPins(const TArray<UEdGraphPin*>& OldPins) const
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

void UK2Node_HasMassFragment::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// 目前 HasMassFragment 没有需要特殊处理的属性
}

//================ Blueprint.Integration				========

void UK2Node_HasMassFragment::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

HNCH_StartExpandNode(UK2Node_HasMassFragment)

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
	UK2Node_CallFunction* HasFunctionNode = nullptr;
	FString FunctionDataSourcePinName;

	switch (OwnerNode->CachedDataSourceType)
	{
		case EMassFragmentSourceDataType::EntityHandle:
			HasFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, HasFragment_Entity_Unified);
			FunctionDataSourcePinName = TEXT("EntityHandle");
			break;
		case EMassFragmentSourceDataType::EntityTemplateData:
			HasFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, HasFragment_Template_Unified);
			FunctionDataSourcePinName = TEXT("TemplateData");
			break;
		default:
			return;
	}

	// 连接参数引脚
	Link(ProxyPin(UK2Node_HasMassFragment::DataSourcePinName()), FunctionInputPin(HasFunctionNode, FunctionDataSourcePinName));
	Link(ProxyPin(UK2Node_HasMassFragment::FragmentTypePinName()), FunctionInputPin(HasFunctionNode, TEXT("FragmentType")));

	// 连接返回值引脚
	UEdGraphPin* FunctionReturnPin = NodePin(HasFunctionNode, UEdGraphSchema_K2::PN_ReturnValue.ToString());
	if (FunctionReturnPin)
	{
		Link(FunctionReturnPin, ProxyPin(UK2Node_HasMassFragment::ReturnValuePinName()));
	}
}

HNCH_EndExpandNode(UK2Node_HasMassFragment)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
