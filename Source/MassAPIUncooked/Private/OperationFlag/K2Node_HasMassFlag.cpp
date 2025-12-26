// Leroy Works & Ember, All Rights Reserved.

#include "OperationFlag/K2Node_HasMassFlag.h"
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

namespace UK2Node_HasMassFlagHelper
{
	const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,				TEXT("HasMassFlag") },
		{ EMassFragmentSourceDataType::EntityHandle,		TEXT("HasMassFlag-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,	TEXT("HasMassFlag-Template") }
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

using namespace UK2Node_HasMassFlagHelper;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

FText UK2Node_HasMassFlag::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_HasMassFlag::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Flag"));
}

FText UK2Node_HasMassFlag::GetTooltipText() const
{
	return FText::FromString(TEXT("Check if an entity or entity template has a flag."));
}

FSlateIcon UK2Node_HasMassFlag::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = DataSourceIconColors.FindRef(CachedDataSourceType);
	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UK2Node_HasMassFlag::GetNodeTitleColor() const
{
	return DataSourceTitleColors.FindRef(CachedDataSourceType);
}

void UK2Node_HasMassFlag::AllocateDefaultPins()
{
	// 纯节点，不需要执行引脚

	// 创建DataSource引脚（Wildcard，支持FEntityHandle和FEntityTemplateData）
	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsConst = true;

	// 创建Flag引脚（EEntityFlags枚举）
	UEdGraphPin* FlagPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte, StaticEnum<EEntityFlags>(), FlagPinName());

	// 创建返回值引脚（bool类型）
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());

	Super::AllocateDefaultPins();
	UpdateDataSourceType();
}

void UK2Node_HasMassFlag::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin && OldPin->PinName == DataSourcePinName())
		{
			if (UEdGraphPin* NewDataSourcePin = FindPin(DataSourcePinName()))
			{
				NewDataSourcePin->PinType = OldPin->PinType;
				NewDataSourcePin->PinType.bIsConst = true;
			}
			break;
		}
	}

	UpdateDataSourceType();
}

void UK2Node_HasMassFlag::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
}

bool UK2Node_HasMassFlag::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	if (MyPin && MyPin->PinName == DataSourcePinName())
	{
		if (OtherPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			OutReason = TEXT("DataSource cannot connect to Wildcard. It must be either FEntityHandle or FEntityTemplateData.");
			return true;
		}

		if (const UScriptStruct* PinStruct = Cast<UScriptStruct>(OtherPin->PinType.PinSubCategoryObject.Get()))
		{
			if (PinStruct != FEntityHandle::StaticStruct() && PinStruct != FEntityTemplateData::StaticStruct())
			{
				OutReason = TEXT("DataSource must be either FEntityHandle or FEntityTemplateData.");
				return true;
			}
		}
		else
		{
			OutReason = TEXT("DataSource must be either FEntityHandle or FEntityTemplateData.");
			return true;
		}
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_HasMassFlag::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin && Pin->PinName == DataSourcePinName())
	{
		UEdGraphPin* DataSourcePin = FindPin(DataSourcePinName());
		if (DataSourcePin)
		{
			if (DataSourcePin->LinkedTo.Num() > 0)
			{
				UEdGraphPin* LinkedPin = DataSourcePin->LinkedTo[0];
				if (LinkedPin && LinkedPin->PinType.PinSubCategoryObject.IsValid())
				{
					DataSourcePin->PinType = LinkedPin->PinType;
					DataSourcePin->PinType.bIsConst = true;
				}
			}
			else
			{
				DataSourcePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
				DataSourcePin->PinType.PinSubCategoryObject = nullptr;
				DataSourcePin->PinType.bIsConst = true;
			}

			UpdateDataSourceType();
			GetGraph()->NotifyNodeChanged(this);
		}
	}
}

void UK2Node_HasMassFlag::UpdateDataSourceType()
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

void UK2Node_HasMassFlag::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UK2Node_HasMassFlag::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

HNCH_StartExpandNode(UK2Node_HasMassFlag)

virtual void Compile() override
{
	OwnerNode->UpdateDataSourceType();

	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		CompilerContext.MessageLog.Error(TEXT("DataSource must be connected to either FEntityHandle or FEntityTemplateData. @@"), OwnerNode);
		return;
	}

	UK2Node_CallFunction* HasFunctionNode = nullptr;
	FString FunctionDataSourcePinName;

	switch (OwnerNode->CachedDataSourceType)
	{
		case EMassFragmentSourceDataType::EntityHandle:
			HasFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, HasFlag_Entity);
			FunctionDataSourcePinName = TEXT("EntityHandle");
			break;
		case EMassFragmentSourceDataType::EntityTemplateData:
			HasFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, HasFlag_Template);
			FunctionDataSourcePinName = TEXT("TemplateData");
			break;
		default:
			return;
	}

	Link(ProxyPin(UK2Node_HasMassFlag::DataSourcePinName()), FunctionInputPin(HasFunctionNode, FunctionDataSourcePinName));
	Link(ProxyPin(UK2Node_HasMassFlag::FlagPinName()), FunctionInputPin(HasFunctionNode, TEXT("FlagToTest")));

	UEdGraphPin* FunctionReturnPin = NodePin(HasFunctionNode, UEdGraphSchema_K2::PN_ReturnValue.ToString());
	if (FunctionReturnPin)
	{
		Link(FunctionReturnPin, ProxyPin(UK2Node_HasMassFlag::ReturnValuePinName()));
	}
}

HNCH_EndExpandNode(UK2Node_HasMassFlag)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
