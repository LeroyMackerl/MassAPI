#include "Common/MagnusNode_Assign.h"

// Engine
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "KismetCompilerMisc.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Styling/AppStyle.h"
#include "EdGraphUtilities.h"
#include "BlueprintCompiledStatement.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration																		========

//———————— Node.Appearance																						————

FText UMagnusNode_Assign::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString("Assign");
}

FText UMagnusNode_Assign::GetCompactNodeTitle() const
{
	return FText::FromString("ASSIGN");
}

FText UMagnusNode_Assign::GetTooltipText() const
{
	return FText::FromString("Set by ref var");
}

FText UMagnusNode_Assign::GetMenuCategory() const
{
	return FText::FromString("Utilities|Magnus");
}

FSlateIcon UMagnusNode_Assign::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Function_16x");
	return Icon;
}

//================ Pin.Management																			========

//———————— Pin.Registry																							————

static FName TargetVarPinName(TEXT("Target"));
UEdGraphPin* UMagnusNode_Assign::GetTargetPin() const
{
	return FindPin(TargetVarPinName);
}

static FName VarValuePinName(TEXT("Value"));
UEdGraphPin* UMagnusNode_Assign::GetValuePin() const
{
	return FindPin(VarValuePinName);
}

//———————— Pin.Construction																						————

void UMagnusNode_Assign::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	UEdGraphNode::FCreatePinParams PinParams;
	PinParams.bIsReference = true;
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, TargetVarPinName, PinParams);

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, VarValuePinName);
}

//———————— Pin.Notify																							————

void UMagnusNode_Assign::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	AllocateDefaultPins();

	UEdGraphPin* OldTargetPin = nullptr;
	for (UEdGraphPin* CurrPin : OldPins)
	{
		if (CurrPin->PinName == TargetVarPinName)
		{
			OldTargetPin = CurrPin;
			break;
		}
	}
  
	if( OldTargetPin )
	{
		UEdGraphPin* NewTargetPin = GetTargetPin();
		CoerceTypeFromPin(OldTargetPin);
	}

	RestoreSplitPins(OldPins);
}

void UMagnusNode_Assign::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	UEdGraphPin* TargetPin = GetTargetPin();
	UEdGraphPin* ValuePin = GetValuePin();

	if( (Pin == TargetPin) || (Pin == ValuePin) )
	{
		UEdGraphPin* ConnectedToPin = (Pin->LinkedTo.Num() > 0) ? Pin->LinkedTo[0] : nullptr;
		CoerceTypeFromPin(ConnectedToPin);

		// If both target and value pins are unlinked, then reset types to wildcard
		if(TargetPin->LinkedTo.Num() == 0 && ValuePin->LinkedTo.Num() == 0)
		{
			// collapse SubPins back into their parent if there are any
			auto TryRecombineSubPins = [](UEdGraphPin* ParentPin)
			{
				if (!ParentPin->SubPins.IsEmpty())
				{
					const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
					K2Schema->RecombinePin(ParentPin->SubPins[0]);
				}
			};
			
			// Pin disconnected...revert to wildcard
			TargetPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			TargetPin->PinType.PinSubCategory = NAME_None;
			TargetPin->PinType.PinSubCategoryObject = nullptr;
			TargetPin->BreakAllPinLinks();
			TryRecombineSubPins(TargetPin);

			ValuePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			ValuePin->PinType.PinSubCategory = NAME_None;
			ValuePin->PinType.PinSubCategoryObject = nullptr;
			ValuePin->BreakAllPinLinks();
			TryRecombineSubPins(ValuePin);
		}
		
		// Get the graph to refresh our title and default value info
		GetGraph()->NotifyNodeChanged(this);
	}
}

void UMagnusNode_Assign::PostPasteNode()
{
	Super::PostPasteNode();

	// 获取引脚
	UEdGraphPin* TargetPin = GetTargetPin();
	UEdGraphPin* ValuePin = GetValuePin();

	// 只有当两个引脚都没有连接时才重置为Wildcard
	if (TargetPin->LinkedTo.Num() == 0 && ValuePin->LinkedTo.Num() == 0)
	{
		// 重置 Target 引脚为 Wildcard
		TargetPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
		TargetPin->PinType.PinSubCategory = NAME_None;
		TargetPin->PinType.PinSubCategoryObject = nullptr;
		if (!TargetPin->SubPins.IsEmpty())
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			K2Schema->RecombinePin(TargetPin->SubPins[0]);
		}

		// 重置 Value 引脚为 Wildcard
		ValuePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
		ValuePin->PinType.PinSubCategory = NAME_None;
		ValuePin->PinType.PinSubCategoryObject = nullptr;
		if (!ValuePin->SubPins.IsEmpty())
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			K2Schema->RecombinePin(ValuePin->SubPins[0]);
		}
	}
}

//———————— Pin.Refresh																							————

void UMagnusNode_Assign::CoerceTypeFromPin(const UEdGraphPin* Pin)
{
	UEdGraphPin* TargetPin = GetTargetPin();
	UEdGraphPin* ValuePin = GetValuePin();

	check(TargetPin && ValuePin);

	if (!Pin || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		return;
	}

	// 当 Target 和 Value 都连接时，使用 Target 的类型
	if (TargetPin->LinkedTo.Num() > 0 && ValuePin->LinkedTo.Num() > 0)
	{
		if (Pin == TargetPin)
		{
			check(Pin->PinType.bIsReference && !Pin->PinType.IsContainer());

			TargetPin->PinType = Pin->PinType;
			TargetPin->PinType.bIsReference = true;

			ValuePin->PinType = Pin->PinType;
			ValuePin->PinType.bIsReference = false;
		}
		return;
	}

	// 当只有一个引脚连接时，两个引脚都使用该引脚的类型
	if (Pin == TargetPin)
	{
		check(Pin->PinType.bIsReference && !Pin->PinType.IsContainer());
	}

	TargetPin->PinType = Pin->PinType;
	TargetPin->PinType.bIsReference = true;

	ValuePin->PinType = Pin->PinType;
	ValuePin->PinType.bIsReference = false;
}

//================ Blueprint.Integration																		========

//———————— Blueprint.Menu																							————

void UMagnusNode_Assign::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

bool UMagnusNode_Assign::IsActionFilteredOut(class FBlueprintActionFilter const& Filter)
{
	// Default to filtering this node out unless dragging off of a reference output pin
	bool bIsFilteredOut = false;
	FBlueprintActionContext const& FilterContext = Filter.Context;

	for (UEdGraphPin* Pin : FilterContext.Pins)
	{
		if(Pin->Direction == EGPD_Output && Pin->PinType.bIsReference == true)
		{
			bIsFilteredOut = false;
			break;
		}
	}
	return bIsFilteredOut;
}

//———————— Blueprint.Compile																						————

class FKCHandler_Assign : public FNodeHandlingFunctor
{
public:
	FKCHandler_Assign(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UMagnusNode_Assign* VarRefNode = CastChecked<UMagnusNode_Assign>(Node);
		UEdGraphPin* ValuePin = VarRefNode->GetValuePin();
		ValidateAndRegisterNetIfLiteral(Context, ValuePin);

		{
			using namespace UE::KismetCompiler;

			UEdGraphPin* VariablePin = VarRefNode->GetTargetPin();
			UEdGraphPin* VariablePinNet = FEdGraphUtilities::GetNetFromPin(VariablePin);
			UEdGraphPin* ValuePinNet = FEdGraphUtilities::GetNetFromPin(ValuePin);

			if ((VariablePinNet != nullptr) && (ValuePinNet != nullptr))
			{
				CastingUtils::FConversion Conversion =
					CastingUtils::GetFloatingPointConversion(*ValuePinNet, *VariablePinNet);

				if (Conversion.Type != CastingUtils::FloatingPointCastType::None)
				{
					check(!ImplicitCastMap.Contains(VarRefNode));

					FBPTerminal* NewTerminal = CastingUtils::MakeImplicitCastTerminal(Context, VariablePinNet);

					ImplicitCastMap.Add(VarRefNode, CastingUtils::FImplicitCastParams{Conversion, NewTerminal, Node});
				}
			}
		}
	}

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UMagnusNode_Assign* VarRefNode = CastChecked<UMagnusNode_Assign>(Node);
		UEdGraphPin* VarTargetPin = VarRefNode->GetTargetPin();
		UEdGraphPin* ValuePin = VarRefNode->GetValuePin();

		InnerAssignment(Context, Node, VarTargetPin, ValuePin);

		// Generate the output impulse from this node
		GenerateSimpleThenGoto(Context, *Node);
	}

private:

	void InnerAssignment(FKismetFunctionContext& Context, UEdGraphNode* Node, UEdGraphPin* VariablePin, UEdGraphPin* ValuePin)
	{
		UEdGraphPin* VariablePinNet = FEdGraphUtilities::GetNetFromPin(VariablePin);
		UEdGraphPin* ValuePinNet = FEdGraphUtilities::GetNetFromPin(ValuePin);

		FBPTerminal** VariableTerm = Context.NetMap.Find(VariablePin);
		if (VariableTerm == nullptr)
		{
			VariableTerm = Context.NetMap.Find(VariablePinNet);
		}

		FBPTerminal** ValueTerm = Context.LiteralHackMap.Find(ValuePin);
		if (ValueTerm == nullptr)
		{
			ValueTerm = Context.NetMap.Find(ValuePinNet);
		}

		if ((VariableTerm != nullptr) && (ValueTerm != nullptr))
		{
			FBPTerminal* LHSTerm = *VariableTerm;
			FBPTerminal* RHSTerm = *ValueTerm;

			{
				using namespace UE::KismetCompiler;

				UMagnusNode_Assign* VarRefNode = CastChecked<UMagnusNode_Assign>(Node);
				if (CastingUtils::FImplicitCastParams* CastParams = ImplicitCastMap.Find(VarRefNode))
				{
					CastingUtils::InsertImplicitCastStatement(Context, *CastParams, RHSTerm);
			
					RHSTerm = CastParams->TargetTerminal;

					ImplicitCastMap.Remove(VarRefNode);

					// We've manually registered our cast statement, so it can be removed from the context.
					CastingUtils::RemoveRegisteredImplicitCast(Context, VariablePin);
					CastingUtils::RemoveRegisteredImplicitCast(Context, ValuePin);
				}
			}

			FBlueprintCompiledStatement& Statement = Context.AppendStatementForNode(Node);
			Statement.Type = KCST_Assignment;
			Statement.LHS = LHSTerm;
			Statement.RHS.Add(RHSTerm);

			if (!(*VariableTerm)->IsTermWritable())
			{
				CompilerContext.MessageLog.Error(*FText::FromString("Cannot write to const @@").ToString(), VariablePin);
			}
		}
		else
		{
			if (VariablePin != ValuePin)
			{
				CompilerContext.MessageLog.Error(*FText::FromString("Failed to resolve term @@ passed into @@").ToString(), ValuePin, VariablePin);
			}
			else
			{
				CompilerContext.MessageLog.Error(*FText::FromString("Failed to resolve term passed into @@").ToString(), VariablePin);
			}
		}
	}

	TMap<UMagnusNode_Assign*, UE::KismetCompiler::CastingUtils::FImplicitCastParams> ImplicitCastMap;
};

UMagnusNode_Assign::UMagnusNode_Assign(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FNodeHandlingFunctor* UMagnusNode_Assign::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_Assign(CompilerContext);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
