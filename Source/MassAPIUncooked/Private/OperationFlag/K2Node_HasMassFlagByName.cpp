/*
* MassAPI
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#include "OperationFlag/K2Node_HasMassFlagByName.h"
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

namespace UK2Node_HasMassFlagByNameHelper
{
	static const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,				TEXT("HasMassFlag") },
		{ EMassFragmentSourceDataType::EntityHandle,		TEXT("HasMassFlag-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,	TEXT("HasMassFlag-Template") }
	};

	static const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceIconColors =
	{
		{ EMassFragmentSourceDataType::None,                FLinearColor(1.0f, 0.9f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,        FLinearColor(1.0f, 0.9f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,  FLinearColor(1.0f, 0.9f, 0.0f, 1.0f) },
	};

	static const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceTitleColors =
	{
		{ EMassFragmentSourceDataType::None,                FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,        FLinearColor(1.0f, 0.9f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,  FLinearColor(1.0f, 0.9f, 0.0f, 1.0f) },
	};
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

FText UK2Node_HasMassFlagByName::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(UK2Node_HasMassFlagByNameHelper::DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_HasMassFlagByName::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Flag"));
}

FText UK2Node_HasMassFlagByName::GetTooltipText() const
{
	return FText::FromString(TEXT("Check if an entity or entity template has a named flag (FName). | 按名称检查旗标。"));
}

FSlateIcon UK2Node_HasMassFlagByName::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = UK2Node_HasMassFlagByNameHelper::DataSourceIconColors.FindRef(CachedDataSourceType);
	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UK2Node_HasMassFlagByName::GetNodeTitleColor() const
{
	return UK2Node_HasMassFlagByNameHelper::DataSourceTitleColors.FindRef(CachedDataSourceType);
}

void UK2Node_HasMassFlagByName::AllocateDefaultPins()
{
	// Pure node, no exec pins | 纯节点，无执行引脚

	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsConst = true;

	// Flag pin — FName type | FName 类型旗标引脚
	UEdGraphPin* FlagPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, NAME_None, FlagPinName());

	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());

	Super::AllocateDefaultPins();
	UpdateDataSourceType();
}

void UK2Node_HasMassFlagByName::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
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

void UK2Node_HasMassFlagByName::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
}

bool UK2Node_HasMassFlagByName::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
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

void UK2Node_HasMassFlagByName::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
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

void UK2Node_HasMassFlagByName::UpdateDataSourceType()
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

void UK2Node_HasMassFlagByName::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UK2Node_HasMassFlagByName::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

HNCH_StartExpandNode(UK2Node_HasMassFlagByName)

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
			HasFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, HasFlag_EntityByName);
			FunctionDataSourcePinName = TEXT("EntityHandle");
			break;
		case EMassFragmentSourceDataType::EntityTemplateData:
			HasFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, HasFlag_TemplateByName);
			FunctionDataSourcePinName = TEXT("TemplateData");
			break;
		default:
			return;
	}

	Link(ProxyPin(UK2Node_HasMassFlagByName::DataSourcePinName()), FunctionInputPin(HasFunctionNode, FunctionDataSourcePinName));
	Link(ProxyPin(UK2Node_HasMassFlagByName::FlagPinName()), FunctionInputPin(HasFunctionNode, TEXT("FlagName")));

	UEdGraphPin* FunctionReturnPin = NodePin(HasFunctionNode, UEdGraphSchema_K2::PN_ReturnValue.ToString());
	if (FunctionReturnPin)
	{
		Link(FunctionReturnPin, ProxyPin(UK2Node_HasMassFlagByName::ReturnValuePinName()));
	}
}

HNCH_EndExpandNode(UK2Node_HasMassFlagByName)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
