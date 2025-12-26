#include "Map/MagnusNode_MapIdentical.h"

// Engine
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "SPinTypeSelector.h"

// Project
#include "NodeCompiler/Magnus_HyperNodeCompilerHandler.h"
#include "FuncLib/MagnusFuncLib_Map.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// namespace MapIdenticalHelper removed since we now use static FName functions

//================ Node.Configuration																		========

//———————— Node.Appearance																						————

FText UMagnusNode_MapIdentical::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("Identical"));
}

FText UMagnusNode_MapIdentical::GetCompactNodeTitle() const
{
	return FText::FromString(TEXT("IDENTICAL"));
}

FText UMagnusNode_MapIdentical::GetTooltipText() const
{
	return FText::FromString(TEXT("Checks whether or not two maps are completely identical."));
}

FText UMagnusNode_MapIdentical::GetMenuCategory() const
{
	return FText::FromString(TEXT("Utilities|Magnus"));
}

FSlateIcon UMagnusNode_MapIdentical::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("EditorStyle", "GraphEditor.PureFunction_16x");
	return Icon;
}

TSharedPtr<SWidget> UMagnusNode_MapIdentical::CreateNodeImage() const
{
	return SPinTypeSelector::ConstructPinTypeImage(FindPin(InputMapAPinName()));
}

//================ Pin.Management																			========


//———————— Pin.Construction																						————

void UMagnusNode_MapIdentical::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Map pin params
	UEdGraphNode::FCreatePinParams PinParams;
	PinParams.ContainerType = EPinContainerType::Map;
	PinParams.ValueTerminalType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;
	PinParams.ValueTerminalType.TerminalSubCategory = NAME_None;
	PinParams.ValueTerminalType.TerminalSubCategoryObject = nullptr;

	// MapA
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, InputMapAPinName(), PinParams);

	// MapB
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, InputMapBPinName(), PinParams);

	// Return type
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, OutputResultPinName());
}

//———————— Pin.Notify																							————

void UMagnusNode_MapIdentical::PostReconstructNode()
{
	Super::PostReconstructNode();

	UEdGraphPin* MapAPin = FindPin(InputMapAPinName());
	UEdGraphPin* MapBPin = FindPin(InputMapBPinName());

	if(MapAPin && MapAPin->LinkedTo.Num() > 0 && MapAPin->LinkedTo[0]->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
	{
		MapAPin->PinType = MapAPin->LinkedTo[0]->PinType;
		MapAPin->PinType.PinValueType = FEdGraphTerminalType(MapAPin->PinType.PinValueType);

		MapBPin->PinType = MapAPin->LinkedTo[0]->PinType;
		MapBPin->PinType.PinValueType = FEdGraphTerminalType(MapAPin->PinType.PinValueType);
	}
	else if(MapBPin && MapBPin->LinkedTo.Num() > 0 && MapBPin->LinkedTo[0]->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
	{
		MapBPin->PinType = MapBPin->LinkedTo[0]->PinType;
		MapBPin->PinType.PinValueType = FEdGraphTerminalType(MapBPin->PinType.PinValueType);

		MapAPin->PinType = MapBPin->LinkedTo[0]->PinType;
		MapAPin->PinType.PinValueType = FEdGraphTerminalType(MapBPin->PinType.PinValueType);
	}
}

void UMagnusNode_MapIdentical::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if(Pin == FindPin(InputMapAPinName()) || Pin == FindPin(InputMapBPinName()))
	{
		if(FindPin(InputMapAPinName())->LinkedTo.Num() == 0 && FindPin(InputMapBPinName())->LinkedTo.Num() == 0)
		{
			FindPin(InputMapAPinName())->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			FindPin(InputMapAPinName())->PinType.PinSubCategory = NAME_None;
			FindPin(InputMapAPinName())->PinType.PinSubCategoryObject = nullptr;
			FindPin(InputMapAPinName())->PinType.PinValueType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;
			FindPin(InputMapAPinName())->PinType.PinValueType.TerminalSubCategory = NAME_None;
			FindPin(InputMapAPinName())->PinType.PinValueType.TerminalSubCategoryObject = nullptr;

			FindPin(InputMapBPinName())->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			FindPin(InputMapBPinName())->PinType.PinSubCategory = NAME_None;
			FindPin(InputMapBPinName())->PinType.PinSubCategoryObject = nullptr;
			FindPin(InputMapBPinName())->PinType.PinValueType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;
			FindPin(InputMapBPinName())->PinType.PinValueType.TerminalSubCategory = NAME_None;
			FindPin(InputMapBPinName())->PinType.PinValueType.TerminalSubCategoryObject = nullptr;
		}
		else
		{
			if(Pin == FindPin(InputMapAPinName()))
			{
				if(FindPin(InputMapAPinName())->LinkedTo.Num() > 0 && FindPin(InputMapAPinName())->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard && FindPin(InputMapAPinName())->LinkedTo[0]->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
				{
					FindPin(InputMapAPinName())->PinType = FindPin(InputMapAPinName())->LinkedTo[0]->PinType;
					FindPin(InputMapAPinName())->PinType.PinValueType = FEdGraphTerminalType(FindPin(InputMapAPinName())->PinType.PinValueType);

					FindPin(InputMapBPinName())->PinType = FindPin(InputMapAPinName())->LinkedTo[0]->PinType;
					FindPin(InputMapBPinName())->PinType.PinValueType = FEdGraphTerminalType(FindPin(InputMapAPinName())->PinType.PinValueType);
				}
			}
			else if(Pin == FindPin(InputMapBPinName()))
			{
				if(FindPin(InputMapBPinName())->LinkedTo.Num() > 0 && FindPin(InputMapBPinName())->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard && FindPin(InputMapBPinName())->LinkedTo[0]->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
				{
					FindPin(InputMapBPinName())->PinType = FindPin(InputMapBPinName())->LinkedTo[0]->PinType;
					FindPin(InputMapBPinName())->PinType.PinValueType = FEdGraphTerminalType(FindPin(InputMapBPinName())->PinType.PinValueType);

					FindPin(InputMapAPinName())->PinType = FindPin(InputMapBPinName())->LinkedTo[0]->PinType;
					FindPin(InputMapAPinName())->PinType.PinValueType = FEdGraphTerminalType(FindPin(InputMapBPinName())->PinType.PinValueType);
				}
			}
		}
		
		GetGraph()->NotifyGraphChanged();
	}
}

//================ Blueprint.Integration																		========

//———————— Blueprint.Menu																							————

void UMagnusNode_MapIdentical::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	const UClass* Action = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(Action))
	{
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);

		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}	
}

//———————— Blueprint.Compile																						————

HNCH_StartExpandNode(UMagnusNode_MapIdentical)

	virtual void Compile() override
	{
		UEdGraphPin* MapAPin = OwnerNode->FindPin(OwnerNode->InputMapAPinName());
		UEdGraphPin* MapBPin = OwnerNode->FindPin(OwnerNode->InputMapBPinName());

		if(MapAPin->PinType != MapBPin->PinType)
		{
			CompilerContext.MessageLog.Error(TEXT("MapA and MapB must be of the same time."), OwnerNode);
			return;
		}

		// MapB GetKeys
		UK2Node_CallFunction* MapBGetKeysNode = HNCH_SpawnFunctionNode(UMagnusNodesFuncLib_MapHelper, Map_Keys);
		Link(MapBPin, FunctionInputPin(MapBGetKeysNode, "TargetMap"));

		// MapB GetValues
		UK2Node_CallFunction* MapBGetValuesNode = HNCH_SpawnFunctionNode(UMagnusNodesFuncLib_MapHelper, Map_Values);
		Link(MapBPin, FunctionInputPin(MapBGetValuesNode, "TargetMap"));

		// Map Identical function call
		UK2Node_CallFunction* MapIdenticalNode = HNCH_SpawnFunctionNode(UMagnusNodesFuncLib_MapHelper, Map_Identical);

        // Link MapA input
		Link(MapAPin, FunctionInputPin(MapIdenticalNode, "MapA"));

        // Link MapB Keys and Values to MapIdentical
		Link(FunctionReturnPin(MapBGetKeysNode, UMagnusNode_MapIdentical::InputKeysPinName().ToString()), FunctionInputPin(MapIdenticalNode, TEXT("KeysB")));
		Link(FunctionReturnPin(MapBGetValuesNode, UMagnusNode_MapIdentical::InputValuesPinName().ToString()), FunctionInputPin(MapIdenticalNode, TEXT("ValuesB")));

        // Link result to OwnerNode's ReturnValue pin
		Link(FunctionReturnPin(MapIdenticalNode), ProxyPin(UMagnusNode_MapIdentical::OutputResultPinName()));
	}

HNCH_EndExpandNode(UMagnusNode_MapIdentical)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
