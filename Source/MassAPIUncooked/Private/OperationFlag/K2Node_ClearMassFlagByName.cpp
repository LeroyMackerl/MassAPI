/*
* MassAPI
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#include "OperationFlag/K2Node_ClearMassFlagByName.h"
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

namespace K2Node_ClearMassFlagByName_Local
{

static void SetDelegatePinType(UEdGraphPin* Pin)
{
	if (!Pin) { return; }
	UFunction* Function = UMassAPIFuncLib::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UMassAPIFuncLib, SetFragment_Entity_Unified));
	if (Function)
	{
		if (FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(Function->FindPropertyByName(TEXT("OnFinished"))))
		{
			Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Delegate;
			Pin->PinType.PinSubCategoryObject = DelegateProp->SignatureFunction;
			if (DelegateProp->SignatureFunction)
			{
				FMemberReference::FillSimpleMemberReference(static_cast<UFunction*>(DelegateProp->SignatureFunction.Get()), Pin->PinType.PinSubCategoryMemberReference);
			}
		}
	}
}

} // namespace K2Node_ClearMassFlagByName_Local

namespace UK2Node_ClearMassFlagByNameHelper
{
	static const TMap<EMassFragmentSourceDataType, FString> DataSourceTypeTitles =
	{
		{ EMassFragmentSourceDataType::None,				TEXT("ClearMassFlag") },
		{ EMassFragmentSourceDataType::EntityHandle,		TEXT("ClearMassFlag-Entity") },
		{ EMassFragmentSourceDataType::EntityTemplateData,	TEXT("ClearMassFlag-Template") }
	};

	static const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceIconColors =
	{
		{ EMassFragmentSourceDataType::None,                FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,        FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,  FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
	};

	static const TMap<EMassFragmentSourceDataType, FLinearColor> DataSourceTitleColors =
	{
		{ EMassFragmentSourceDataType::None,                FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityHandle,        FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
		{ EMassFragmentSourceDataType::EntityTemplateData,  FLinearColor(1.0f, 0.147f, 0.1f, 1.0f) },
	};
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration			========

//———————— Node.Appearance						————

FText UK2Node_ClearMassFlagByName::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(UK2Node_ClearMassFlagByNameHelper::DataSourceTypeTitles.FindRef(CachedDataSourceType));
}

FText UK2Node_ClearMassFlagByName::GetMenuCategory() const
{
	return FText::FromString(TEXT("MassAPI|Flag"));
}

FText UK2Node_ClearMassFlagByName::GetTooltipText() const
{
	return FText::FromString(TEXT("Clear a named flag (FName) from an entity or entity template. | 按名称清除旗标。"));
}

FSlateIcon UK2Node_ClearMassFlagByName::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = UK2Node_ClearMassFlagByNameHelper::DataSourceIconColors.FindRef(CachedDataSourceType);

	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

FLinearColor UK2Node_ClearMassFlagByName::GetNodeTitleColor() const
{
	return UK2Node_ClearMassFlagByNameHelper::DataSourceTitleColors.FindRef(CachedDataSourceType);
}

//================ Pin.Management				========

//———————— Pin.Construction					————

void UK2Node_ClearMassFlagByName::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	UEdGraphPin* DataSourcePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DataSourcePinName());
	DataSourcePin->PinType.bIsReference = true;

	// Flag pin — FName type | FName 类型旗标引脚
	UEdGraphPin* FlagPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, NAME_None, FlagPinName());

	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());

	UEdGraphPin* DeferredPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, DeferredPinName());
	DeferredPin->DefaultValue = TEXT("false");

	Super::AllocateDefaultPins();

	UpdateDataSourceType();
}

//———————— Pin.Modification					————

void UK2Node_ClearMassFlagByName::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

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

		K2Node_ClearMassFlagByName_Local::SetDelegatePinType(DelegatePin);

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

	UpdateDataSourceType();
}

void UK2Node_ClearMassFlagByName::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdateDataSourceType();
}

//------------------------ Pin.ValueChange ------------------------ | 引脚值变更 ------------------------

void UK2Node_ClearMassFlagByName::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	if (Pin && Pin->PinName == DeferredPinName())
	{
		const bool bIsChecked = (Pin->DefaultValue == TEXT("true"));
		UEdGraphPin* DelegatePin = FindPin(OnFinishedPinName());

		bool bChanged = false;

		if (bIsChecked && !DelegatePin)
		{
			DelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, OnFinishedPinName());
			K2Node_ClearMassFlagByName_Local::SetDelegatePinType(DelegatePin);
			bChanged = true;
		}
		else if (!bIsChecked && DelegatePin)
		{
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

//———————— Pin.Connection						————

bool UK2Node_ClearMassFlagByName::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
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

void UK2Node_ClearMassFlagByName::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
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
					DataSourcePin->PinType.bIsReference = true;
				}
			}
			else
			{
				DataSourcePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
				DataSourcePin->PinType.PinSubCategoryObject = nullptr;
				DataSourcePin->PinType.bIsReference = true;
			}

			UpdateDataSourceType();
			GetGraph()->NotifyNodeChanged(this);
		}
	}
}

//================ DataSource.Access			========

void UK2Node_ClearMassFlagByName::UpdateDataSourceType()
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

	UEdGraphPin* ReturnPin = FindPin(ReturnValuePinName());

	if (CachedDataSourceType == EMassFragmentSourceDataType::EntityTemplateData)
	{
		if (ReturnPin)
		{
			ReturnPin->BreakAllPinLinks();
			Pins.Remove(ReturnPin);
		}
	}
	else if (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle)
	{
		if (!ReturnPin)
		{
			CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());
		}
	}
	else if (CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		if (!ReturnPin)
		{
			CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ReturnValuePinName());
		}
	}

	UEdGraphPin* DeferredPin = FindPin(DeferredPinName());
	if (DeferredPin)
	{
		const bool bShow = (CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle);

		if (DeferredPin->bHidden == bShow)
		{
			DeferredPin->bHidden = !bShow;
			if (!bShow)
			{
				DeferredPin->BreakAllPinLinks();
			}
		}

		UEdGraphPin* DelegatePin = FindPin(OnFinishedPinName());
		if (DelegatePin && !bShow)
		{
			DelegatePin->BreakAllPinLinks();
			RemovePin(DelegatePin);
		}
	}
}

//================ Editor.PropertyChange		========

void UK2Node_ClearMassFlagByName::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

//================ Blueprint.Integration		========

void UK2Node_ClearMassFlagByName::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

HNCH_StartExpandNode(UK2Node_ClearMassFlagByName)

virtual void Compile() override
{
	OwnerNode->UpdateDataSourceType();

	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::None)
	{
		CompilerContext.MessageLog.Error(TEXT("DataSource must be connected to either FEntityHandle or FEntityTemplateData. @@"), OwnerNode);
		return;
	}

	UK2Node_CallFunction* ClearFunctionNode = nullptr;
	FString FunctionDataSourcePinName;

	switch (OwnerNode->CachedDataSourceType)
	{
		case EMassFragmentSourceDataType::EntityHandle:
			ClearFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, ClearFlag_EntityByName);
			FunctionDataSourcePinName = TEXT("EntityHandle");
			break;
		case EMassFragmentSourceDataType::EntityTemplateData:
			ClearFunctionNode = HNCH_SpawnFunctionNode(UMassAPIFuncLib, ClearFlag_TemplateByName);
			FunctionDataSourcePinName = TEXT("TemplateData");
			break;
		default:
			return;
	}

	Link(ProxyExecPin(), ExecPin(ClearFunctionNode));
	Link(ThenPin(ClearFunctionNode), ProxyThenPin());

	Link(ProxyPin(UK2Node_ClearMassFlagByName::DataSourcePinName()), FunctionInputPin(ClearFunctionNode, FunctionDataSourcePinName));
	Link(ProxyPin(UK2Node_ClearMassFlagByName::FlagPinName()), FunctionInputPin(ClearFunctionNode, TEXT("FlagName")));

	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle)
	{
		Link(ProxyPin(UK2Node_ClearMassFlagByName::DeferredPinName()), FunctionInputPin(ClearFunctionNode, TEXT("bDeferred")));
		if (UEdGraphPin* DelegatePin = ProxyPin(UK2Node_ClearMassFlagByName::OnFinishedPinName()))
		{
			Link(DelegatePin, FunctionInputPin(ClearFunctionNode, TEXT("OnFinished")));
		}
	}

	if (OwnerNode->CachedDataSourceType == EMassFragmentSourceDataType::EntityHandle)
	{
		UEdGraphPin* FunctionReturnPin = NodePin(ClearFunctionNode, UEdGraphSchema_K2::PN_ReturnValue.ToString());
		if (FunctionReturnPin)
		{
			Link(FunctionReturnPin, ProxyPin(UK2Node_ClearMassFlagByName::ReturnValuePinName()));
		}
	}
}

HNCH_EndExpandNode(UK2Node_ClearMassFlagByName)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
