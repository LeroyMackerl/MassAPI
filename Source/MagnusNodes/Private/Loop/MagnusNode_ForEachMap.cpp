#include "Loop/MagnusNode_ForEachMap.h"

// Engine
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/BlueprintMapLibrary.h"
#include "SPinTypeSelector.h"

// 编译器需要的节点类
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_TemporaryVariable.h"
#include "FuncLib/MagnusFuncLib_Map.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration																		========

//———————— Node.Appearance																						————

FText UMagnusNode_ForEachMap::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (NodeTitleText.ToString() == TEXT("Default"))
	{
		FString Title;

		// 如果是反向遍历，则在开头添加"Reverse"
		if (Reverse)
		{
			Title = TEXT("Reverse For Each Map");
		}
		else
		{
			Title = TEXT("For Each Map");
		}

		// 如果支持中断，则在末尾添加"With Break"
		if (CouldBreak)
		{
			Title += TEXT(" With Break");
		}

		return FText::FromString(Title);
	}
	return NodeTitleText;
}

FText UMagnusNode_ForEachMap::GetTooltipText() const
{
	return FText::FromString(TEXT("Loops through each key value pair in the map."));
}

FText UMagnusNode_ForEachMap::GetMenuCategory() const
{
	return FText::FromString(TEXT("Utilities|Magnus"));
}

FSlateIcon UMagnusNode_ForEachMap::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor(0.9f,0.3f,0.15f);
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Macro.ForEach_16x");
	return Icon;
}

FLinearColor UMagnusNode_ForEachMap::GetNodeTitleColor() const
{
	return FLinearColor(0.9f,0.3f,0.15f);
}


//================ Node.Properties																			========

void UMagnusNode_ForEachMap::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	ReconstructNode();
	GetGraph()->NotifyGraphChanged();
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

//================ Pin.Management																			========

//———————— Pin.Registry																							————

const FName UMagnusNode_ForEachMap::LoopBodyPinName(TEXT("Loop Body"));
UEdGraphPin* UMagnusNode_ForEachMap::GetLoopBodyPin() const
{
	return FindPin(LoopBodyPinName);
}

const FName UMagnusNode_ForEachMap::BreakPinName(TEXT("Break"));
UEdGraphPin* UMagnusNode_ForEachMap::GetBreakPin() const
{
	return FindPin(BreakPinName);
}

const FName UMagnusNode_ForEachMap::MapPinName(TEXT("Map"));
UEdGraphPin* UMagnusNode_ForEachMap::GetMapPin() const
{
	return FindPin(MapPinName);
}

const FName UMagnusNode_ForEachMap::KeyPinName(TEXT("Key"));
UEdGraphPin* UMagnusNode_ForEachMap::GetKeyPin() const
{
	return FindPin(KeyPinName);
}

const FName UMagnusNode_ForEachMap::ValuePinName(TEXT("Value"));
UEdGraphPin* UMagnusNode_ForEachMap::GetValuePin() const
{
	return FindPin(ValuePinName);
}

const FName UMagnusNode_ForEachMap::IndexPinName(TEXT("Index"));
UEdGraphPin* UMagnusNode_ForEachMap::GetIndexPin() const
{
	return FindPin(IndexPinName);
}

UEdGraphPin* UMagnusNode_ForEachMap::GetCompletedPin() const
{
	return FindPinChecked(UEdGraphSchema_K2::PN_Then, EGPD_Output);
}

//———————— Pin.Construction																						————

void UMagnusNode_ForEachMap::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Execute
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

	// Map 
	UEdGraphNode::FCreatePinParams PinParams;
	PinParams.ContainerType = EPinContainerType::Map;
	PinParams.ValueTerminalType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;
	PinParams.ValueTerminalType.TerminalSubCategory = NAME_None;
	PinParams.ValueTerminalType.TerminalSubCategoryObject = nullptr;
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, MapPinName, PinParams);

	// Loop body
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, LoopBodyPinName);

	UEdGraphPin* BreakPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, BreakPinName);
	BreakPin->PinFriendlyName = FText::FromName(BreakPinName);
	BreakPin->bHidden = !CouldBreak;

	// 根据遍历方向决定引脚创建顺序
	if (Reverse)
	{
		// 反向遍历时先显示索引，再显示值
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, IndexPinName);
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, KeyPinName);
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, ValuePinName);
	}
	else
	{
		// 正向遍历时先显示值，再显示索引
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, KeyPinName);
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, ValuePinName);
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, IndexPinName);
	}
	
	// Completed
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then)->PinFriendlyName = FText::FromName(UEdGraphSchema_K2::PN_Completed);

	RefreshPinType();
}

//———————— Pin.Notify																							————

void UMagnusNode_ForEachMap::PostReconstructNode()
{
	Super::PostReconstructNode();

	RefreshBreakPin();
	RefreshPinType();
}

void UMagnusNode_ForEachMap::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin == GetMapPin() || Pin == GetKeyPin() || Pin == GetValuePin())
	{
		RefreshPinType();
	}
}

//———————— Pin.Refresh																							————

void UMagnusNode_ForEachMap::RefreshBreakPin() const
{
	UEdGraphPin* BreakPin = GetBreakPin();
	if (!CouldBreak && BreakPin && BreakPin->LinkedTo.Num() > 0)
	{
		BreakPin->BreakAllPinLinks(true);
	}
}

void UMagnusNode_ForEachMap::RefreshPinType() const
{
    bool bNotifyGraphChanged = false;
    UEdGraphPin* MapPin = GetMapPin();
    UEdGraphPin* KeyPin = GetKeyPin();
    UEdGraphPin* ValuePin = GetValuePin();

	// 第一种情况：Map引脚无连接
	if (MapPin->LinkedTo.Num() == 0)
	{
		// 初始化 Map 引脚为 Wildcard
		MapPin->PinType.ContainerType = EPinContainerType::Map;
		MapPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
		MapPin->PinType.PinSubCategory = NAME_None;
		MapPin->PinType.PinSubCategoryObject = nullptr;
		MapPin->PinType.PinValueType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;
		MapPin->PinType.PinValueType.TerminalSubCategory = NAME_None;
		MapPin->PinType.PinValueType.TerminalSubCategoryObject = nullptr;

		// 处理 Key 引脚
		if (KeyPin->LinkedTo.Num() > 0)
		{
			const UEdGraphPin* ConnectedKeyPin = KeyPin->LinkedTo[0];
			MapPin->PinType.PinCategory = ConnectedKeyPin->PinType.PinCategory;
			MapPin->PinType.PinSubCategory = ConnectedKeyPin->PinType.PinSubCategory;
			MapPin->PinType.PinSubCategoryObject = ConnectedKeyPin->PinType.PinSubCategoryObject;
			KeyPin->PinType = ConnectedKeyPin->PinType;
		}
		else
		{
			// 重置未连接的 Key 引脚
			KeyPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			KeyPin->PinType.PinSubCategory = NAME_None;
			KeyPin->PinType.PinSubCategoryObject = nullptr;
		}

		// 处理 Value 引脚
		if (ValuePin->LinkedTo.Num() > 0)
		{
			const UEdGraphPin* ConnectedValuePin = ValuePin->LinkedTo[0];
			MapPin->PinType.PinValueType.TerminalCategory = ConnectedValuePin->PinType.PinCategory;
			MapPin->PinType.PinValueType.TerminalSubCategory = ConnectedValuePin->PinType.PinSubCategory;
			MapPin->PinType.PinValueType.TerminalSubCategoryObject = ConnectedValuePin->PinType.PinSubCategoryObject;
			ValuePin->PinType = ConnectedValuePin->PinType;
		}
		else
		{
			// 重置未连接的 Value 引脚
			ValuePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			ValuePin->PinType.PinSubCategory = NAME_None;
			ValuePin->PinType.PinSubCategoryObject = nullptr;
		}

		bNotifyGraphChanged = true;
	}
	
	// 第二种情况：Map引脚有连接且需要更新类型
	else if (MapPin->LinkedTo.Num() > 0)
	{
	    const UEdGraphPin* ConnectedPin = MapPin->LinkedTo[0];
	    
	    if (ConnectedPin->PinType.ContainerType == EPinContainerType::Map)
	    {
	        bool bShouldUpdate = false;
	        
	        // 更新 Map 引脚的容器类型
	        if (MapPin->PinType.ContainerType != EPinContainerType::Map)
	        {
	            MapPin->PinType.ContainerType = EPinContainerType::Map;
	            bShouldUpdate = true;
	        }

	        // 处理 Key 类型
	        if (KeyPin->LinkedTo.Num() > 0 && KeyPin->LinkedTo[0]->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
	        {
	            // 使用已连接的 Key 引脚类型
	            const UEdGraphPin* ConnectedKeyPin = KeyPin->LinkedTo[0];
	            MapPin->PinType.PinCategory = ConnectedKeyPin->PinType.PinCategory;
	            MapPin->PinType.PinSubCategory = ConnectedKeyPin->PinType.PinSubCategory;
	            MapPin->PinType.PinSubCategoryObject = ConnectedKeyPin->PinType.PinSubCategoryObject;
	            KeyPin->PinType = ConnectedKeyPin->PinType;
	            bShouldUpdate = true;
	        }
	        else if (ConnectedPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
	        {
	        	// 使用外部 Map 的 Key 类型（而不是整个 Map 的类型）
	        	MapPin->PinType.PinCategory = ConnectedPin->PinType.PinCategory;
	        	MapPin->PinType.PinSubCategory = ConnectedPin->PinType.PinSubCategory;
	        	MapPin->PinType.PinSubCategoryObject = ConnectedPin->PinType.PinSubCategoryObject;
    
	        	KeyPin->PinType.PinCategory = ConnectedPin->PinType.PinCategory;
	        	KeyPin->PinType.PinSubCategory = ConnectedPin->PinType.PinSubCategory;
	        	KeyPin->PinType.PinSubCategoryObject = ConnectedPin->PinType.PinSubCategoryObject;
	        	bShouldUpdate = true;
	        }

	        // 处理 Value 类型
	        if (ValuePin->LinkedTo.Num() > 0 && ValuePin->LinkedTo[0]->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
	        {
	            // 使用已连接的 Value 引脚类型
	            const UEdGraphPin* ConnectedValuePin = ValuePin->LinkedTo[0];
	            MapPin->PinType.PinValueType.TerminalCategory = ConnectedValuePin->PinType.PinCategory;
	            MapPin->PinType.PinValueType.TerminalSubCategory = ConnectedValuePin->PinType.PinSubCategory;
	            MapPin->PinType.PinValueType.TerminalSubCategoryObject = ConnectedValuePin->PinType.PinSubCategoryObject;
	            ValuePin->PinType = ConnectedValuePin->PinType;
	            bShouldUpdate = true;
	        }
	        else if (ConnectedPin->PinType.PinValueType.TerminalCategory != UEdGraphSchema_K2::PC_Wildcard)
	        {
	            // 使用外部 Map 的 Value 类型
	            const FEdGraphPinType ValuePinType = FEdGraphPinType::GetPinTypeForTerminalType(ConnectedPin->PinType.PinValueType);
	            MapPin->PinType.PinValueType = ConnectedPin->PinType.PinValueType;
	            ValuePin->PinType = ValuePinType;
	            bShouldUpdate = true;
	        }

	        if (bShouldUpdate)
	        {
	            bNotifyGraphChanged = true;
	        }
	    }
	}

    if (bNotifyGraphChanged)
    {
        GetGraph()->NotifyGraphChanged();
    }
}

//———————— Pin.Connection																						————

bool UMagnusNode_ForEachMap::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
    if (Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason))
    {
        return true;
    }

    // 只检查 Map 引脚的连接
    if (MyPin == GetMapPin() || (OtherPin->PinType.ContainerType == EPinContainerType::Map && OtherPin->Direction != MyPin->Direction))
    {
        const UEdGraphPin* MapPin = (MyPin == GetMapPin()) ? MyPin : OtherPin;
        const UEdGraphPin* OtherMapPin = (MyPin == GetMapPin()) ? OtherPin : MyPin;

        // 确保另一个引脚也是 Map 类型
        if (OtherMapPin->PinType.ContainerType != EPinContainerType::Map)
        {
            OutReason = TEXT("目标引脚必须是 Map 类型");
            return true;
        }

        // 检查 Key 类型是否匹配（如果不是 Wildcard）
        if (MapPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard &&
            OtherMapPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard &&
            (MapPin->PinType.PinCategory != OtherMapPin->PinType.PinCategory ||
             MapPin->PinType.PinSubCategory != OtherMapPin->PinType.PinSubCategory ||
             MapPin->PinType.PinSubCategoryObject != OtherMapPin->PinType.PinSubCategoryObject))
        {
            OutReason = TEXT("Map 的 Key 类型不匹配");
            return true;
        }

        // 检查 Value 类型是否匹配（如果不是 Wildcard）
        if (MapPin->PinType.PinValueType.TerminalCategory != UEdGraphSchema_K2::PC_Wildcard &&
            OtherMapPin->PinType.PinValueType.TerminalCategory != UEdGraphSchema_K2::PC_Wildcard &&
            (MapPin->PinType.PinValueType.TerminalCategory != OtherMapPin->PinType.PinValueType.TerminalCategory ||
             MapPin->PinType.PinValueType.TerminalSubCategory != OtherMapPin->PinType.PinValueType.TerminalSubCategory ||
             MapPin->PinType.PinValueType.TerminalSubCategoryObject != OtherMapPin->PinType.PinValueType.TerminalSubCategoryObject))
        {
            OutReason = TEXT("Map 的 Value 类型不匹配");
            return true;
        }
    }

    return false;
}

//================ Blueprint.Integration																		========

//———————— Blueprint.Menu																							————

void UMagnusNode_ForEachMap::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

void UMagnusNode_ForEachMap::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	// 检查 Map 引脚是否连接且类型有效
	UEdGraphPin* MapPin = GetMapPin();
	if (MapPin->LinkedTo.Num() == 0)
	{
		CompilerContext.MessageLog.Error(TEXT("请连接 Map 引脚 @@"), this);
		return;
	}

	// 检查 Map 的 Key 和 Value 类型是否有效
	if (MapPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard ||
		MapPin->PinType.PinValueType.TerminalCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		CompilerContext.MessageLog.Error(TEXT("Map 的 Key 和 Value 类型必须有效 @@"), this);
		return;
	}

	// 缓存常用类引用
	static UClass* MathLibClass = nullptr;
	static UClass* MapLibClass = nullptr;
	static UClass* ExtNodeFuncsClass = nullptr;

	if (!MathLibClass) MathLibClass = UKismetMathLibrary::StaticClass();
	if (!MapLibClass) MapLibClass = UBlueprintMapLibrary::StaticClass();
	if (!ExtNodeFuncsClass) ExtNodeFuncsClass = UMagnusNodesFuncLib_MapHelper::StaticClass();

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

	// 一次性预先创建所有中间节点
	struct FNodesCollection {
		UK2Node_TemporaryVariable* LoopCounterNode;
		UK2Node_AssignmentStatement* LoopCounterInitialise;
		UK2Node_IfThenElse* Branch;
		UK2Node_CallFunction* Condition;
		UK2Node_CallFunction* Length;
		UK2Node_AssignmentStatement* LoopCounterBreak;
		UK2Node_ExecutionSequence* Sequence;
		UK2Node_CallFunction* Increment;
		UK2Node_AssignmentStatement* LoopCounterAssign;
		UK2Node_CallFunction* GetPair;
		// 反向遍历可能需要的额外节点
		UK2Node_CallFunction* LengthMinus1;
	} Nodes;

	// 创建所有中间节点
	Nodes.LoopCounterNode = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(this, SourceGraph);
	Nodes.LoopCounterInitialise = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	Nodes.Branch = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
	Nodes.Condition = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	Nodes.Length = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	Nodes.LoopCounterBreak = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	Nodes.Sequence = CompilerContext.SpawnIntermediateNode<UK2Node_ExecutionSequence>(this, SourceGraph);
	Nodes.Increment = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	Nodes.LoopCounterAssign = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	Nodes.GetPair = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

	// 循环计数器
	Nodes.LoopCounterNode->VariableType.PinCategory = UEdGraphSchema_K2::PC_Int;
	Nodes.LoopCounterNode->AllocateDefaultPins();
	UEdGraphPin* LoopCounterPin = Nodes.LoopCounterNode->GetVariablePin();

	// 获取Map长度
	Nodes.Length->SetFromFunction(UBlueprintMapLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBlueprintMapLibrary, Map_Length)));
	Nodes.Length->AllocateDefaultPins();
	UEdGraphPin* LengthTargetMapPin = Nodes.Length->FindPinChecked(TEXT("TargetMap"), EGPD_Input);
	LengthTargetMapPin->PinType = MapPin->PinType;
	LengthTargetMapPin->PinType.PinValueType = FEdGraphTerminalType(MapPin->PinType.PinValueType);

	bool bResult = true;

	// 根据Reverse为真或假配置不同的循环逻辑
	if (Reverse)
	{
		// 初始化循环计数器为数组长度-1
		Nodes.LengthMinus1 = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		Nodes.LengthMinus1->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt)));
		Nodes.LengthMinus1->AllocateDefaultPins();
		bResult &= Schema->TryCreateConnection(Nodes.LengthMinus1->FindPinChecked(TEXT("A")), Nodes.Length->GetReturnValuePin());
		Nodes.LengthMinus1->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("1");

		// 设置初始值
		Nodes.LoopCounterInitialise->AllocateDefaultPins();
		bResult &= Schema->TryCreateConnection(Nodes.LoopCounterInitialise->GetValuePin(), Nodes.LengthMinus1->GetReturnValuePin());

		// 循环条件：判断索引是否大于等于0
		Nodes.Condition->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_IntInt)));
		Nodes.Condition->AllocateDefaultPins();
		Nodes.Condition->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("0");

		// 设置递减逻辑
		Nodes.Increment->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt)));
		Nodes.Increment->AllocateDefaultPins();
		Nodes.Increment->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("1");

		// Break：设为-1
		Nodes.LoopCounterBreak->AllocateDefaultPins();
		Nodes.LoopCounterBreak->GetValuePin()->DefaultValue = TEXT("-1");
	}
	else
	{
		// 正常循环：从0开始
		Nodes.LoopCounterInitialise->AllocateDefaultPins();
		Nodes.LoopCounterInitialise->GetValuePin()->DefaultValue = TEXT("0");

		// 循环条件：判断索引是否小于Map长度
		Nodes.Condition->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_IntInt)));
		Nodes.Condition->AllocateDefaultPins();
		bResult &= Schema->TryCreateConnection(Nodes.Condition->FindPinChecked(TEXT("B")), Nodes.Length->GetReturnValuePin());

		// 设置递增逻辑
		Nodes.Increment->SetFromFunction(MathLibClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt)));
		Nodes.Increment->AllocateDefaultPins();
		Nodes.Increment->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("1");

		// Break：设为Map长度
		Nodes.LoopCounterBreak->AllocateDefaultPins();
		bResult &= Schema->TryCreateConnection(Nodes.LoopCounterBreak->GetValuePin(), Nodes.Length->GetReturnValuePin());
	}

	UEdGraphPin* LoopCounterInitialiseExecPin = Nodes.LoopCounterInitialise->GetExecPin();

	// 连接循环计数器
	bResult &= Schema->TryCreateConnection(LoopCounterPin, Nodes.LoopCounterInitialise->GetVariablePin());
	if (!bResult) CompilerContext.MessageLog.Error(TEXT("无法连接初始化循环计数器节点"));

	// 分支
	Nodes.Branch->AllocateDefaultPins();
	bResult &= Schema->TryCreateConnection(Nodes.LoopCounterInitialise->GetThenPin(), Nodes.Branch->GetExecPin());
	UEdGraphPin* BranchElsePin = Nodes.Branch->GetElsePin();

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("无法连接分支节点"));

	// 条件连接
	bResult &= Schema->TryCreateConnection(Nodes.Condition->GetReturnValuePin(), Nodes.Branch->GetConditionPin());
	bResult &= Schema->TryCreateConnection(Nodes.Condition->FindPinChecked(TEXT("A")), LoopCounterPin);

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("无法连接循环条件节点"));

	// Break连接
	bResult &= Schema->TryCreateConnection(Nodes.LoopCounterBreak->GetVariablePin(), LoopCounterPin);
	UEdGraphPin* LoopCounterBreakExecPin = Nodes.LoopCounterBreak->GetExecPin();

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("无法连接Break节点"));

	// 序列
	Nodes.Sequence->AllocateDefaultPins();
	bResult &= Schema->TryCreateConnection(Nodes.Sequence->GetExecPin(), Nodes.Branch->GetThenPin());
	UEdGraphPin* SequenceThen0Pin = Nodes.Sequence->GetThenPinGivenIndex(0);
	UEdGraphPin* SequenceThen1Pin = Nodes.Sequence->GetThenPinGivenIndex(1);

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("无法连接序列节点"));

	// 递增/递减处理
	bResult &= Schema->TryCreateConnection(Nodes.Increment->FindPinChecked(TEXT("A")), LoopCounterPin);

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("无法连接循环计数器递增/递减节点"));

	// 循环计数器赋值
	Nodes.LoopCounterAssign->AllocateDefaultPins();
	bResult &= Schema->TryCreateConnection(Nodes.LoopCounterAssign->GetExecPin(), SequenceThen1Pin);
	bResult &= Schema->TryCreateConnection(Nodes.LoopCounterAssign->GetVariablePin(), LoopCounterPin);
	bResult &= Schema->TryCreateConnection(Nodes.LoopCounterAssign->GetValuePin(), Nodes.Increment->GetReturnValuePin());
	bResult &= Schema->TryCreateConnection(Nodes.LoopCounterAssign->GetThenPin(), Nodes.Branch->GetExecPin());

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("无法连接循环计数器赋值节点"));

	// 获取Map键值对
	Nodes.GetPair->SetFromFunction(ExtNodeFuncsClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UMagnusNodesFuncLib_MapHelper, MagnusLoop_Map_GetPair)));
	Nodes.GetPair->AllocateDefaultPins();

	// 设置正确的GetPair函数参数类型
	UEdGraphPin* GetPairMapPin = Nodes.GetPair->FindPinChecked(TEXT("TargetMap"), EGPD_Input);
	GetPairMapPin->PinType.ResetToDefaults();
	GetPairMapPin->PinType.PinCategory = MapPin->PinType.PinCategory;
	GetPairMapPin->PinType.PinSubCategory = MapPin->PinType.PinSubCategory;
	GetPairMapPin->PinType.PinSubCategoryObject = MapPin->PinType.PinSubCategoryObject;
	GetPairMapPin->PinType.ContainerType = MapPin->PinType.ContainerType;
	GetPairMapPin->PinType.PinValueType.TerminalCategory = MapPin->PinType.PinValueType.TerminalCategory;
	GetPairMapPin->PinType.PinValueType.TerminalSubCategory = MapPin->PinType.PinValueType.TerminalSubCategory;
	GetPairMapPin->PinType.PinValueType.TerminalSubCategoryObject = MapPin->PinType.PinValueType.TerminalSubCategoryObject;

	// 同样更新Key和Value引脚的类型
	UEdGraphPin* KeyPin = Nodes.GetPair->FindPinChecked(TEXT("Key"));
	KeyPin->PinType.ResetToDefaults();
	KeyPin->PinType.PinCategory = MapPin->PinType.PinCategory;
	KeyPin->PinType.PinSubCategory = MapPin->PinType.PinSubCategory;
	KeyPin->PinType.PinSubCategoryObject = MapPin->PinType.PinSubCategoryObject;

	UEdGraphPin* ValuePin = Nodes.GetPair->FindPinChecked(TEXT("Value"));
	ValuePin->PinType.ResetToDefaults();
	ValuePin->PinType.PinCategory = MapPin->PinType.PinValueType.TerminalCategory;
	ValuePin->PinType.PinSubCategory = MapPin->PinType.PinValueType.TerminalSubCategory;
	ValuePin->PinType.PinSubCategoryObject = MapPin->PinType.PinValueType.TerminalSubCategoryObject;

	// 连接Index引脚到循环计数器
	UEdGraphPin* GetPairIndexPin = Nodes.GetPair->FindPinChecked(TEXT("Index"), EGPD_Input);
	bResult &= Schema->TryCreateConnection(GetPairIndexPin, LoopCounterPin);

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("无法连接GetPair的Index引脚"));

	// 连接GetPair的执行引脚
	UEdGraphPin* GetPairExecPin = Nodes.GetPair->GetExecPin();
	UEdGraphPin* GetPairThenPin = Nodes.GetPair->GetThenPin();
	bResult &= Schema->TryCreateConnection(GetPairExecPin, SequenceThen0Pin);

	if (!bResult) CompilerContext.MessageLog.Error(TEXT("无法连接获取Map键值对节点"));

	// 一次性批量连接引脚
	CompilerContext.CopyPinLinksToIntermediate(*MapPin, *LengthTargetMapPin);
	CompilerContext.CopyPinLinksToIntermediate(*MapPin, *GetPairMapPin);

	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *LoopCounterInitialiseExecPin);
	CompilerContext.MovePinLinksToIntermediate(*GetLoopBodyPin(), *GetPairThenPin);
	CompilerContext.MovePinLinksToIntermediate(*GetCompletedPin(), *BranchElsePin);
	CompilerContext.MovePinLinksToIntermediate(*GetBreakPin(), *LoopCounterBreakExecPin);
	CompilerContext.MovePinLinksToIntermediate(*GetKeyPin(), *KeyPin);
	CompilerContext.MovePinLinksToIntermediate(*GetValuePin(), *ValuePin);
	CompilerContext.MovePinLinksToIntermediate(*GetIndexPin(), *LoopCounterPin);

	// 重构需要的节点
	TArray<UK2Node_CallFunction*> NodesToReconstruct = { Nodes.Length, Nodes.GetPair };
	if (Reverse) NodesToReconstruct.Add(Nodes.LengthMinus1);
	for (UK2Node_CallFunction* Node : NodesToReconstruct)
	{
		Node->PostReconstructNode();
	}

	BreakAllNodeLinks();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————