#include "Loop/MagnusNode_ForEachSet.h"

// Engine
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/BlueprintSetLibrary.h"
#include "SPinTypeSelector.h"

// 编译器需要的节点类
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_TemporaryVariable.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration																		========

//———————— Node.Appearance																						————

FText UMagnusNode_ForEachSet::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (NodeTitleText.ToString() == TEXT("Default"))
	{
		FString Title = TEXT("For Each Set");

		// 如果支持中断，则在末尾添加"With Break"
		if (CouldBreak)
		{
			Title += TEXT(" With Break");
		}

		return FText::FromString(Title);
	}

	return NodeTitleText;
}


FText UMagnusNode_ForEachSet::GetTooltipText() const
{
	return FText::FromString(TEXT("Loops through each value in the set."));
}

FText UMagnusNode_ForEachSet::GetMenuCategory() const
{
	return FText::FromString(TEXT("Utilities|Magnus"));
}

FSlateIcon UMagnusNode_ForEachSet::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor(0.75f, 0.95f, 0.2f);
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Macro.ForEach_16x");
	return Icon;
}

FLinearColor UMagnusNode_ForEachSet::GetNodeTitleColor() const
{
	return FLinearColor(0.75f, 0.95f, 0.2f);
}


//================ Node.Properties																			========

void UMagnusNode_ForEachSet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

//================ Pin.Management																			========

//———————— Pin.Registry																							————

const FName UMagnusNode_ForEachSet::LoopBodyPinName(TEXT("Loop Body"));
UEdGraphPin* UMagnusNode_ForEachSet::GetLoopBodyPin() const
{
	return FindPin(LoopBodyPinName);
}

const FName UMagnusNode_ForEachSet::BreakPinName(TEXT("Break"));
UEdGraphPin* UMagnusNode_ForEachSet::GetBreakPin() const
{
	return FindPin(BreakPinName);
}

const FName UMagnusNode_ForEachSet::SetPinName(TEXT("Set"));
UEdGraphPin* UMagnusNode_ForEachSet::GetSetPin() const
{
	return FindPin(SetPinName);
}

const FName UMagnusNode_ForEachSet::ValuePinName(TEXT("Value"));
UEdGraphPin* UMagnusNode_ForEachSet::GetValuePin() const
{
	return FindPin(ValuePinName);
}

const FName UMagnusNode_ForEachSet::IndexPinName(TEXT("Index"));
UEdGraphPin* UMagnusNode_ForEachSet::GetIndexPin() const
{
	return FindPin(IndexPinName);
}

UEdGraphPin* UMagnusNode_ForEachSet::GetCompletedPin() const
{
	return FindPinChecked(UEdGraphSchema_K2::PN_Then, EGPD_Output);
}

//———————— Pin.Construction																						————

void UMagnusNode_ForEachSet::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Execute
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

	// Set 
	UEdGraphNode::FCreatePinParams PinParams;
	PinParams.ContainerType = EPinContainerType::Set;
	PinParams.ValueTerminalType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;
	PinParams.ValueTerminalType.TerminalSubCategory = NAME_None;
	PinParams.ValueTerminalType.TerminalSubCategoryObject = nullptr;
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, SetPinName, PinParams);

	// Break
	UEdGraphPin* BreakPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, BreakPinName);
	BreakPin->PinFriendlyName = FText::FromName(BreakPinName);
	BreakPin->bHidden = !CouldBreak;
	
	// Loop body
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, LoopBodyPinName);

	// Value
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, ValuePinName);

	// Index
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, IndexPinName);

	// Completed
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then)->PinFriendlyName = FText::FromName(UEdGraphSchema_K2::PN_Completed);

	RefreshPinType();
}

//———————— Pin.Notify																							————

void UMagnusNode_ForEachSet::PostReconstructNode()
{
	Super::PostReconstructNode();

	RefreshBreakPin();
	RefreshPinType();
}

void UMagnusNode_ForEachSet::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin == GetSetPin() || Pin == GetValuePin())
	{
		RefreshPinType();
	}
}

//———————— Pin.Refresh																							————

void UMagnusNode_ForEachSet::RefreshBreakPin() const
{
	UEdGraphPin* BreakPin = GetBreakPin();
	if (!CouldBreak && BreakPin && BreakPin->LinkedTo.Num() > 0)
	{
		BreakPin->BreakAllPinLinks(true);
	}
}

void UMagnusNode_ForEachSet::RefreshPinType() const
{
    bool bNotifyGraphChanged = false;
    UEdGraphPin* SetPin = GetSetPin();
    UEdGraphPin* ValuePin = GetValuePin();

	// 当 Set 引脚已连接时，保持其类型不变
	if (SetPin->LinkedTo.Num() > 0)
	{
		const UEdGraphPin* ConnectedPin = SetPin->LinkedTo[0];
		if (ConnectedPin && ConnectedPin->PinType.ContainerType == EPinContainerType::Set)
		{
			// 更新 Set 引脚
			SetPin->PinType.PinCategory = ConnectedPin->PinType.PinCategory;
			SetPin->PinType.PinSubCategory = ConnectedPin->PinType.PinSubCategory;
			SetPin->PinType.PinSubCategoryObject = ConnectedPin->PinType.PinSubCategoryObject;
			SetPin->PinType.ContainerType = EPinContainerType::Set;

			// 更新 Value 引脚
			ValuePin->PinType.PinCategory = ConnectedPin->PinType.PinCategory;
			ValuePin->PinType.PinSubCategory = ConnectedPin->PinType.PinSubCategory;
			ValuePin->PinType.PinSubCategoryObject = ConnectedPin->PinType.PinSubCategoryObject;

			bNotifyGraphChanged = true;
			return;
		}
	}
	
    // 无连接的情况：重置为Wildcard并断开连接
    if (SetPin->LinkedTo.Num() == 0 && ValuePin->LinkedTo.Num() == 0)
    {
        // 重置 Set 引脚
        SetPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
        SetPin->PinType.PinSubCategory = NAME_None;
        SetPin->PinType.PinSubCategoryObject = nullptr;
        SetPin->PinType.PinValueType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;
        SetPin->PinType.PinValueType.TerminalSubCategory = NAME_None;
        SetPin->PinType.PinValueType.TerminalSubCategoryObject = nullptr;
        SetPin->BreakAllPinLinks(true);

        // 重置 Value 引脚
        ValuePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
        ValuePin->PinType.PinSubCategory = NAME_None;
        ValuePin->PinType.PinSubCategoryObject = nullptr;
        ValuePin->BreakAllPinLinks(true);

        bNotifyGraphChanged = true;
    }
    
    // 只有 Set 引脚有连接：同时更新 Set 和 Value 引脚
    else if (SetPin->LinkedTo.Num() > 0 && ValuePin->LinkedTo.Num() == 0)
    {
        if (SetPin->LinkedTo[0]->PinType.ContainerType == EPinContainerType::Set)
        {
            // 更新 Set 引脚
            SetPin->PinType.PinCategory = SetPin->LinkedTo[0]->PinType.PinCategory;
            SetPin->PinType.PinSubCategory = SetPin->LinkedTo[0]->PinType.PinSubCategory;
            SetPin->PinType.PinSubCategoryObject = SetPin->LinkedTo[0]->PinType.PinSubCategoryObject;
            SetPin->PinType.ContainerType = EPinContainerType::Set;
            
            // 更新 Value 引脚
            ValuePin->PinType.PinCategory = SetPin->LinkedTo[0]->PinType.PinCategory;
            ValuePin->PinType.PinSubCategory = SetPin->LinkedTo[0]->PinType.PinSubCategory;
            ValuePin->PinType.PinSubCategoryObject = SetPin->LinkedTo[0]->PinType.PinSubCategoryObject;
            
            bNotifyGraphChanged = true;
        }
    }

    // 只有 Value 引脚有连接：同时更新 Set 和 Value 引脚
    else if (SetPin->LinkedTo.Num() == 0 && ValuePin->LinkedTo.Num() > 0)
    {
        // 更新 Set 引脚
        SetPin->PinType.PinCategory = ValuePin->LinkedTo[0]->PinType.PinCategory;
        SetPin->PinType.PinSubCategory = ValuePin->LinkedTo[0]->PinType.PinSubCategory;
        SetPin->PinType.PinSubCategoryObject = ValuePin->LinkedTo[0]->PinType.PinSubCategoryObject;
        SetPin->PinType.ContainerType = EPinContainerType::Set;
        
        // 更新 Value 引脚
        ValuePin->PinType.PinCategory = ValuePin->LinkedTo[0]->PinType.PinCategory;
        ValuePin->PinType.PinSubCategory = ValuePin->LinkedTo[0]->PinType.PinSubCategory;
        ValuePin->PinType.PinSubCategoryObject = ValuePin->LinkedTo[0]->PinType.PinSubCategoryObject;
        
        bNotifyGraphChanged = true;
    }
    
    // 两个引脚都有连接：不做处理
    if (bNotifyGraphChanged)
    {
        GetGraph()->NotifyGraphChanged();
    }
}

//———————— Pin.Connection																						————

bool UMagnusNode_ForEachSet::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

//================ Blueprint.Integration																		========

//———————— Blueprint.Menu																							————

void UMagnusNode_ForEachSet::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

void UMagnusNode_ForEachSet::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	// 检查 Set 引脚是否连接
	UEdGraphPin* SetPin = GetSetPin();
	if (SetPin->LinkedTo.Num() == 0)
	{
		CompilerContext.MessageLog.Error(TEXT("请连接 Set 引脚 @@"), this);
		return;
	}

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

	bool bResult = true;
	// Create int Loop Counter
	UK2Node_TemporaryVariable* LoopCounterNode = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(this, SourceGraph);
	LoopCounterNode->VariableType.PinCategory = UEdGraphSchema_K2::PC_Int;
	LoopCounterNode->AllocateDefaultPins();
	UEdGraphPin* LoopCounterPin = LoopCounterNode->GetVariablePin();
	check(LoopCounterPin);

	// Initialise loop counter
	UK2Node_AssignmentStatement* LoopCounterInitialise = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	LoopCounterInitialise->AllocateDefaultPins();
	LoopCounterInitialise->GetValuePin()->DefaultValue = TEXT("0");
	bResult &= Schema->TryCreateConnection(LoopCounterPin, LoopCounterInitialise->GetVariablePin());
	UEdGraphPin* LoopCounterInitialiseExecPin = LoopCounterInitialise->GetExecPin();
	check(LoopCounterInitialiseExecPin);

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("Could not connect initialise loop counter node."));

	// Do loop branch
	UK2Node_IfThenElse* Branch = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
	Branch->AllocateDefaultPins();
	bResult &= Schema->TryCreateConnection(LoopCounterInitialise->GetThenPin(), Branch->GetExecPin());
	UEdGraphPin* BranchElsePin = Branch->GetElsePin();
	check(BranchElsePin);

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("Could not connect branch node."));

	// Do loop condition
	UK2Node_CallFunction* Condition = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	Condition->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_IntInt)));
	Condition->AllocateDefaultPins();
	bResult &= Schema->TryCreateConnection(Condition->GetReturnValuePin(), Branch->GetConditionPin());
	bResult &= Schema->TryCreateConnection(Condition->FindPinChecked(TEXT("A")), LoopCounterPin);
	UEdGraphPin* LoopConditionBPin = Condition->FindPinChecked(TEXT("B"));
	check(LoopConditionBPin);

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("Could not connect loop condition node."));

	// Length of Set
	UK2Node_CallFunction* Length = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	Length->SetFromFunction(UBlueprintSetLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBlueprintSetLibrary, Set_Length)));
	Length->AllocateDefaultPins();
	UEdGraphPin* LengthTargetArrayPin = Length->FindPinChecked(TEXT("TargetSet"), EGPD_Input);
	LengthTargetArrayPin->PinType = GetSetPin()->PinType;
	LengthTargetArrayPin->PinType.PinValueType = FEdGraphTerminalType(GetSetPin()->PinType.PinValueType);
	bResult &= Schema->TryCreateConnection(LoopConditionBPin, Length->GetReturnValuePin());
	CompilerContext.CopyPinLinksToIntermediate(*GetSetPin(), *LengthTargetArrayPin);
	Length->PostReconstructNode();

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("Could not connect length node."));

	// Break loop from set counter
	UK2Node_CallFunction* BreakLength = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	BreakLength->SetFromFunction(UBlueprintSetLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBlueprintSetLibrary, Set_Length)));
	BreakLength->AllocateDefaultPins();
	UEdGraphPin* BreakLengthTargetArrayPin = BreakLength->FindPinChecked(TEXT("TargetSet"), EGPD_Input);
	BreakLengthTargetArrayPin->PinType = GetSetPin()->PinType;
	BreakLengthTargetArrayPin->PinType.PinValueType = FEdGraphTerminalType(GetSetPin()->PinType.PinValueType);
	CompilerContext.CopyPinLinksToIntermediate(*GetSetPin(), *BreakLengthTargetArrayPin);
	BreakLength->PostReconstructNode();

	UK2Node_AssignmentStatement* LoopCounterBreak = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	LoopCounterBreak->AllocateDefaultPins();
	bResult &= Schema->TryCreateConnection(LoopCounterBreak->GetVariablePin(), LoopCounterPin);
	bResult &= Schema->TryCreateConnection(LoopCounterBreak->GetValuePin(), BreakLength->GetReturnValuePin());
	UEdGraphPin* LoopCounterBreakExecPin = LoopCounterBreak->GetExecPin();
	check(LoopCounterBreakExecPin);
	if (!bResult) CompilerContext.MessageLog.Error(TEXT("Could not set BreakNode from length node."));

	// Sequence
	UK2Node_ExecutionSequence* Sequence = CompilerContext.SpawnIntermediateNode<UK2Node_ExecutionSequence>(this, SourceGraph);
	Sequence->AllocateDefaultPins();
	bResult &= Schema->TryCreateConnection(Sequence->GetExecPin(), Branch->GetThenPin());
	UEdGraphPin* SequenceThen0Pin = Sequence->GetThenPinGivenIndex(0);
	UEdGraphPin* SequenceThen1Pin = Sequence->GetThenPinGivenIndex(1);
	check(SequenceThen0Pin);
	check(SequenceThen1Pin);

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("Could not connect sequence node."));

	// Loop Counter increment
	UK2Node_CallFunction* Increment = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	Increment->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt)));
	Increment->AllocateDefaultPins();
	bResult &= Schema->TryCreateConnection(Increment->FindPinChecked(TEXT("A")), LoopCounterPin);
	Increment->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("1");

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("Could not connect loop counter increment node."));

	// Loop Counter assigned
	UK2Node_AssignmentStatement* LoopCounterAssign = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	LoopCounterAssign->AllocateDefaultPins();
	bResult &= Schema->TryCreateConnection(LoopCounterAssign->GetExecPin(), SequenceThen1Pin);
	bResult &= Schema->TryCreateConnection(LoopCounterAssign->GetVariablePin(), LoopCounterPin);
	bResult &= Schema->TryCreateConnection(LoopCounterAssign->GetValuePin(), Increment->GetReturnValuePin());
	bResult &= Schema->TryCreateConnection(LoopCounterAssign->GetThenPin(), Branch->GetExecPin());

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("Could not connect loop counter assignment node."));

	// Set to Array
	UK2Node_CallFunction* ToArrayFun = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	ToArrayFun->SetFromFunction(UBlueprintSetLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBlueprintSetLibrary, Set_ToArray)));
	ToArrayFun->AllocateDefaultPins();
	UEdGraphPin* ToArrayFunPin = ToArrayFun->FindPinChecked(TEXT("A"), EGPD_Input);
	ToArrayFunPin->PinType = GetSetPin()->PinType;
	ToArrayFunPin->PinType.PinValueType = FEdGraphTerminalType(GetSetPin()->PinType.PinValueType);
	CompilerContext.CopyPinLinksToIntermediate(*GetSetPin(), *ToArrayFunPin);
	UEdGraphPin* ToArrayFunValuePin = ToArrayFun->FindPinChecked(TEXT("Result"));
	check(ToArrayFunValuePin);
	ToArrayFun->PostReconstructNode();
	bResult &= Schema->TryCreateConnection(ToArrayFun->GetThenPin(), LoopCounterInitialiseExecPin);

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("Could not connect Set to Array node."));

	// Get value
	UK2Node_CallFunction* GetValue = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	GetValue->SetFromFunction(UKismetArrayLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Get)));
	GetValue->AllocateDefaultPins();
	UEdGraphPin* GetValueTargetArrayPin = GetValue->FindPinChecked(TEXT("TargetArray"), EGPD_Input);
	GetValueTargetArrayPin->PinType = ToArrayFunValuePin->PinType;
	GetValueTargetArrayPin->PinType.PinValueType = FEdGraphTerminalType(ToArrayFunValuePin->PinType.PinValueType);
	bResult &= Schema->TryCreateConnection(GetValue->FindPinChecked(TEXT("Index")), LoopCounterPin);
	bResult &= Schema->TryCreateConnection(GetValueTargetArrayPin, ToArrayFunValuePin);
	UEdGraphPin* ValuePin = GetValue->FindPinChecked(TEXT("Item"));
	ValuePin->PinType = GetValuePin()->PinType;
	check(ValuePin);
	GetValue->PostReconstructNode();

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("Could not connect get Set value node."));

	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *(ToArrayFun->GetExecPin()));
	CompilerContext.MovePinLinksToIntermediate(*GetLoopBodyPin(), *SequenceThen0Pin);
	CompilerContext.MovePinLinksToIntermediate(*GetCompletedPin(), *BranchElsePin);
	CompilerContext.MovePinLinksToIntermediate(*GetBreakPin(), *LoopCounterBreakExecPin);
	CompilerContext.MovePinLinksToIntermediate(*GetValuePin(), *ValuePin);
	CompilerContext.MovePinLinksToIntermediate(*GetIndexPin(), *LoopCounterPin);

	BreakAllNodeLinks();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
