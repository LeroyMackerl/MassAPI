// Leroy Works & Ember, All Rights Reserved.

#include "OperationFlag/K2Node_ClearMassFlag.h"
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

namespace UK2Node_ClearMassFlagHelper
{
	// DataSource类型到标题的映射
	const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,				TEXT("ClearMassFlag") },
		{ EMassFragmentSourceDataType::EntityHandle,		TEXT("ClearMassFlag-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,	TEXT("ClearMassFlag-Template") }
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

using namespace UK2Node_ClearMassFlagHelper;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration			========

//———————— Node.Appearance						————

FText UK2Node_ClearMassFlag::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_ClearMassFlag::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Flag"));
}

FText UK2Node_ClearMassFlag::GetTooltipText() const
{
	return FText::FromString(TEXT("Clear a flag from an entity or entity template."));
}

FSlateIcon UK2Node_ClearMassFlag::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = DataSourceIconColors.FindRef(CachedDataSourceType);

	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UK2Node_ClearMassFlag::GetNodeTitleColor() const
{
	return DataSourceTitleColors.FindRef(CachedDataSourceType);
}

//================ Pin.Management				========

//———————— Pin.Construction					————

void UK2Node_ClearMassFlag::AllocateDefaultPins()
{
	// 创建执行引脚
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// 创建DataSource引脚（Wildcard，支持FEntityHandle和FEntityTemplateData）
	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsReference = true;

	// 创建Flag引脚（EEntityFlags枚举）
	UEdGraphPin* FlagPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte, StaticEnum<EEntityFlags>(), FlagPinName());

	// 创建返回值引脚（bool类型，仅EntityHandle模式有效）
	// Template模式下会在UpdateDataSourceType中移除
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());

	Super::AllocateDefaultPins();

	// 初始化DataSource类型
	UpdateDataSourceType();
}

//———————— Pin.Modification					————

void UK2Node_ClearMassFlag::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
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
}

void UK2Node_ClearMassFlag::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
}

//———————— Pin.Connection						————

bool UK2Node_ClearMassFlag::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
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

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_ClearMassFlag::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
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
}

//================ DataSource.Access			========

void UK2Node_ClearMassFlag::UpdateDataSourceType()
{
	UEdGraphPin* DataSourcePin = FindPin(DataSourcePinName());
	if (!DataSourcePin)
	{
		CachedDataSourceType = EMassFragmentSourceDataType::None;
		return;
	}

	UScriptStruct* DataSourceStruct = Cast<UScriptStruct>(DataSourcePin->PinType.PinSubCategoryObject.Get());

	EMassFragmentSourceDataType OldType = CachedDataSourceType;

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

	// 根据DataSource类型管理返回值引脚
	// EntityHandle模式：有返回值
	// Template模式：无返回值
	UEdGraphPin* ReturnPin = FindPin(ReturnValuePinName());

	if (CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateData)
	{
		// Template模式：移除返回值引脚
		if (ReturnPin)
		{
			ReturnPin->BreakAllPinLinks();
			Pins.Remove(ReturnPin);
		}
	}
	else if (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle)
	{
		// EntityHandle模式：确保有返回值引脚
		if (!ReturnPin)
		{
			CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());
		}
	}
	else if (CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		// None模式：确保有返回值引脚（默认状态）
		if (!ReturnPin)
		{
			CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());
		}
	}
}

//================ Editor.PropertyChange		========

void UK2Node_ClearMassFlag::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// 目前 ClearMassFlag 没有需要特殊处理的属性
}

//================ Blueprint.Integration		========

void UK2Node_ClearMassFlag::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

//================ Compiler.Integration			========

HNCH_StartExpandNode(UK2Node_ClearMassFlag)

virtual void Compile() override
{
	// 在编译前再次更新DataSourceType，确保类型正确
	OwnerNode->UpdateDataSourceType();

	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		CompilerContext.MessageLog.Error(TEXT("DataSource must be connected to either FEntityHandle or FEntityTemplateData. @@"), OwnerNode);
		return;
	}

	// 根据DataSourceType选择对应的函数
	UK2Node_CallFunction* ClearFunctionNode = nullptr;
	FString FunctionDataSourcePinName;

	switch (OwnerNode->CachedDataSourceType)
	{
		case EMassFragmentSourceDataType::EntityHandle:
			ClearFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, ClearFlag_Entity);
			FunctionDataSourcePinName = TEXT("EntityHandle");
			break;
		case EMassFragmentSourceDataType::EntityTemplateData:
			ClearFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, ClearFlag_Template);
			FunctionDataSourcePinName = TEXT("TemplateData");
			break;
		default:
			return;
	}

	// 连接执行流
	Link(ProxyExecPin(), ExecPin(ClearFunctionNode));
	Link(ThenPin(ClearFunctionNode), ProxyThenPin());

	// 连接参数引脚
	Link(ProxyPin(UK2Node_ClearMassFlag::DataSourcePinName()), FunctionInputPin(ClearFunctionNode, FunctionDataSourcePinName));
	Link(ProxyPin(UK2Node_ClearMassFlag::FlagPinName()), FunctionInputPin(ClearFunctionNode, TEXT("FlagToClear")));

	// 连接返回值引脚（仅EntityHandle模式）
	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle)
	{
		UEdGraphPin* FunctionReturnPin = NodePin(ClearFunctionNode, UEdGraphSchema_K2::PN_ReturnValue.ToString());
		if (FunctionReturnPin)
		{
			Link(FunctionReturnPin, ProxyPin(UK2Node_ClearMassFlag::ReturnValuePinName()));
		}
	}
}

HNCH_EndExpandNode(UK2Node_ClearMassFlag)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
