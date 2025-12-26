// Leroy Works & Ember, All Rights Reserved.

#include "MassAPIUncooked/Public/OperationTag/K2Node_HasMassTag.h"
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

namespace UK2Node_HasMassTagHelper
{
	const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,				TEXT("HasMassTag") },
		{ EMassFragmentSourceDataType::EntityHandle,		TEXT("HasMassTag-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,	TEXT("HasMassTag-Template") }
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

using namespace UK2Node_HasMassTagHelper;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

FText UK2Node_HasMassTag::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_HasMassTag::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Tag"));
}

FText UK2Node_HasMassTag::GetTooltipText() const
{
	return FText::FromString(TEXT("Check if an entity or entity template has a tag."));
}

FSlateIcon UK2Node_HasMassTag::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = DataSourceIconColors.FindRef(CachedDataSourceType);
	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UK2Node_HasMassTag::GetNodeTitleColor() const
{
	return DataSourceTitleColors.FindRef(CachedDataSourceType);
}

void UK2Node_HasMassTag::AllocateDefaultPins()
{
	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsConst = true;

	UEdGraphPin* TagTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, UScriptStruct::StaticClass(), TagTypePinName());
	TagTypePin->PinType.PinSubCategory = UEdGraphSchema_K2::PSC_Self;

	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());

	Super::AllocateDefaultPins();
	UpdateDataSourceType();
}

void UK2Node_HasMassTag::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
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

	UScriptStruct* OldTagStruct = GetTagStructFromOldPins(OldPins);
	if (OldTagStruct)
	{
		if (UEdGraphPin* TagTypePin = FindPin(TagTypePinName()))
		{
			TagTypePin->DefaultObject = OldTagStruct;
		}
	}
}

void UK2Node_HasMassTag::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
	OnTagTypeChanged();
}

void UK2Node_HasMassTag::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
	if (Pin && Pin->PinName == TagTypePinName())
	{
		OnTagTypeChanged();
	}
}

bool UK2Node_HasMassTag::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
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

void UK2Node_HasMassTag::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
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

	if (Pin && Pin->PinName == TagTypePinName())
	{
		if (Pin->LinkedTo.Num() == 0)
		{
			OnTagTypeChanged();
		}
	}
}

void UK2Node_HasMassTag::UpdateDataSourceType()
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

void UK2Node_HasMassTag::OnTagTypeChanged()
{
	GetGraph()->NotifyNodeChanged(this);
}

bool UK2Node_HasMassTag::IsValidTagStruct(const UScriptStruct* Struct) const
{
	if (!Struct)
	{
		return false;
	}

	return Struct->IsChildOf(FMassTag::StaticStruct());
}

UScriptStruct* UK2Node_HasMassTag::GetTagStruct() const
{
	UEdGraphPin* TagTypePin = FindPin(TagTypePinName());
	if (TagTypePin && TagTypePin->DefaultObject != nullptr && TagTypePin->LinkedTo.Num() == 0)
	{
		return Cast<UScriptStruct>(TagTypePin->DefaultObject);
	}
	return nullptr;
}

UScriptStruct* UK2Node_HasMassTag::GetTagStructFromOldPins(const TArray<UEdGraphPin*>& OldPins) const
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

void UK2Node_HasMassTag::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UK2Node_HasMassTag::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

HNCH_StartExpandNode(UK2Node_HasMassTag)

virtual void Compile() override
{
	OwnerNode->UpdateDataSourceType();

	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		CompilerContext.MessageLog.Error(TEXT("DataSource must be connected to either FEntityHandle or FEntityTemplateData. @@"), OwnerNode);
		return;
	}

	UScriptStruct* TagStruct = OwnerNode->GetTagStruct();
	if (!TagStruct)
	{
		CompilerContext.MessageLog.Error(TEXT("TagType must be specified. Please select a valid Mass Tag type. @@"), OwnerNode);
		return;
	}

	UK2Node_CallFunction* HasFunctionNode = nullptr;
	FString FunctionDataSourcePinName;

	switch (OwnerNode->CachedDataSourceType)
	{
		case EMassFragmentSourceDataType::EntityHandle:
			HasFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, HasTag_Entity);
			FunctionDataSourcePinName = TEXT("EntityHandle");
			break;
		case EMassFragmentSourceDataType::EntityTemplateData:
			HasFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, HasTag_Template);
			FunctionDataSourcePinName = TEXT("TemplateData");
			break;
		default:
			return;
	}

	Link(ProxyPin(UK2Node_HasMassTag::DataSourcePinName()), FunctionInputPin(HasFunctionNode, FunctionDataSourcePinName));
	Link(ProxyPin(UK2Node_HasMassTag::TagTypePinName()), FunctionInputPin(HasFunctionNode, TEXT("TagType")));

	UEdGraphPin* FunctionReturnPin = NodePin(HasFunctionNode, UEdGraphSchema_K2::PN_ReturnValue.ToString());
	if (FunctionReturnPin)
	{
		Link(FunctionReturnPin, ProxyPin(UK2Node_HasMassTag::ReturnValuePinName()));
	}
}

HNCH_EndExpandNode(UK2Node_HasMassTag)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
