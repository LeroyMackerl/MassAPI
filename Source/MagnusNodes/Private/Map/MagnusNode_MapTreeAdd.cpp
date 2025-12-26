#include "Map/MagnusNode_MapTreeAdd.h"

// Editor
#include "EdGraphSchema_K2.h"

// Blueprint
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "SPinTypeSelector.h"
#include "Kismet2/BlueprintEditorUtils.h"

// Project
#include "FuncLib/MagnusFuncLib_Map.h"
#include "NodeCompiler/Magnus_HyperNodeCompilerHandler.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Node.Configuration																		========

//———————— Node.Appearance																						————

FText UMagnusNode_MapTreeAdd::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	switch (ContainerType)
	{
		case EMapTreeContainerType::NoContainer:
			return FText::FromString(TEXT("MapTreeAdd"));
		case EMapTreeContainerType::Array:
			return FText::FromString(TEXT("AddItem-Array"));
		case EMapTreeContainerType::Map:
			return FText::FromString(TEXT("AddItem-Map"));
		case EMapTreeContainerType::Set:
			return FText::FromString(TEXT("AddItem-Set"));
		default:
			return FText::FromString(TEXT("MapTreeAdd"));
	}
}

FText UMagnusNode_MapTreeAdd::GetCompactNodeTitle() const
{
	if (ContainerType == EMapTreeContainerType::NoContainer)
	{
		return FText::FromString(TEXT("MAPTREE"));
	}
	return FText::FromString(TEXT("ADDITEM"));
}

FText UMagnusNode_MapTreeAdd::GetTooltipText() const
{
	switch (ContainerType)
	{
		case EMapTreeContainerType::NoContainer:
			return FText::FromString(TEXT("Map tree operation node. Select container type in details panel."));
		case EMapTreeContainerType::Array:
			return FText::FromString(TEXT("Add a item to array in map struct value"));
		case EMapTreeContainerType::Map:
			return FText::FromString(TEXT("Add a item to map in map struct value"));
		case EMapTreeContainerType::Set:
			return FText::FromString(TEXT("Add a item to set in map struct value"));
		default:
			return FText::FromString(TEXT("Map tree operation"));
	}
}

FText UMagnusNode_MapTreeAdd::GetMenuCategory() const
{
	return FText::FromString(TEXT("Utilities|Magnus"));
}

FSlateIcon UMagnusNode_MapTreeAdd::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("EditorStyle", "GraphEditor.MakeMap_16x");
	return Icon;
}

TSharedPtr<SWidget> UMagnusNode_MapTreeAdd::CreateNodeImage() const
{
	return SPinTypeSelector::ConstructPinTypeImage(FindPin(InputMapPinName()));
}

//================ Pin.Management																			========

//———————— Pin.Construction																						————

void UMagnusNode_MapTreeAdd::AllocateDefaultPins()
{
	// 创建执行引脚（所有模式都有）
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// 创建TargetMap引脚（所有模式都有，包括NoContainer）
	UEdGraphNode::FCreatePinParams PinParams;
	UEdGraphPin* MapPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, InputMapPinName(), PinParams);
	MapPin->PinType.ContainerType = EPinContainerType::Map;

	// 在UE5.4中，使用这种方式设置Map的键值类型
	FEdGraphTerminalType WildcardType;
	WildcardType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;
	MapPin->PinType.PinValueType = WildcardType;
	MapPin->PinType.PinValueType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;

	// 设置引脚友好名称
	MapPin->PinFriendlyName = FText::FromString(TEXT("Target"));

	// 只有在非NoContainer模式下才创建数据引脚
	if (ContainerType != EMapTreeContainerType::NoContainer)
	{
		// 创建 Key 输入引脚
		UEdGraphPin* KeyPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, InputKeyPinName());
		KeyPin->PinType.ContainerType = EPinContainerType::None;
		KeyPin->PinFriendlyName = FText::FromString(TEXT("Key"));

		// 创建 SubKey 输入引脚（仅Map模式）
		if (ContainerType == EMapTreeContainerType::Map)
		{
			UEdGraphPin* SubKeyPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, InputSubKeyPinName());
			SubKeyPin->PinType.ContainerType = EPinContainerType::None;
			SubKeyPin->PinFriendlyName = FText::FromString(TEXT("SubKey"));
		}

		// 创建 Value 输入引脚
		UEdGraphPin* ValuePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, InputItemPinName());
		ValuePin->PinType.ContainerType = EPinContainerType::None;
		ValuePin->PinFriendlyName = FText::FromString(TEXT("Value"));
	}

	Super::AllocateDefaultPins();
}

//———————— Pin.Notify																							————

void UMagnusNode_MapTreeAdd::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
	PropagatePinType();
}

void UMagnusNode_MapTreeAdd::ReconstructNode()
{
	Super::ReconstructNode();
	PropagatePinType();
}

void UMagnusNode_MapTreeAdd::PostReconstructNode()
{
	Super::PostReconstructNode();
	PropagatePinType();
}

void UMagnusNode_MapTreeAdd::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	// 如果是 Map 引脚被改变
	if (Pin && Pin->PinName == InputMapPinName())
	{
		// 如果 Map 引脚有连接
		if (Pin->LinkedTo.Num() > 0)
		{
			// 自动设置 MemberReference.StructType
			const FEdGraphPinType& MapPinType = Pin->LinkedTo[0]->PinType;
			if (MapPinType.ContainerType == EPinContainerType::Map &&
				MapPinType.PinValueType.TerminalCategory == UEdGraphSchema_K2::PC_Struct)
			{
				UScriptStruct* StructType = Cast<UScriptStruct>(MapPinType.PinValueType.TerminalSubCategoryObject.Get());
				if (StructType)
				{
					// 设置 StructType（触发自定义面板更新）
					MemberReference.StructType = StructType;

					// 如果 MemberReference 已经有 MemberPath，验证它是否仍然有效
					if (!MemberReference.MemberPath.IsEmpty())
					{
						FProperty* Property = StructMemberHelper::FindPropertyBySinglePath(
							MemberReference.StructType,
							MemberReference.MemberPath
						);

						// 如果找不到该成员，清空 MemberPath
						if (!Property)
						{
							MemberReference.MemberPath.Empty();
							ContainerType = EMapTreeContainerType::NoContainer;
						}
					}
				}
			}
		}
		else
		{
			// 如果 Map 引脚没有连接，断开其他引脚并重置
			if (UEdGraphPin* KeyPin = FindPin(InputKeyPinName()))
			{
				KeyPin->BreakAllPinLinks();
			}

			if (UEdGraphPin* SubKeyPin = FindPin(InputSubKeyPinName()))
			{
				SubKeyPin->BreakAllPinLinks();
			}

			if (UEdGraphPin* ValuePin = FindPin(InputItemPinName()))
			{
				ValuePin->BreakAllPinLinks();
			}

			// 重置 MemberReference
			MemberReference.Reset();
			ContainerType = EMapTreeContainerType::NoContainer;
		}
	}

	// 更新引脚类型
	PropagatePinType();
}

void UMagnusNode_MapTreeAdd::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);
	PropagatePinType();
}

//———————— Pin.Connection																						————

bool UMagnusNode_MapTreeAdd::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	// 先检查参数有效性
	if (!MyPin || !OtherPin)
	{
		return false;
	}

	// 如果是 Map 引脚，执行 Map 相关的检查
	if (MyPin->PinName == InputMapPinName())
	{
		// Map 类型检查
		if (OtherPin->PinType.ContainerType != EPinContainerType::Map)
		{
			OutReason = TEXT("目标引脚必须是Map类型");
			return true;
		}

		// Value 类型检查
		if (OtherPin->PinType.PinValueType.TerminalCategory != UEdGraphSchema_K2::PC_Struct)
		{
			OutReason = TEXT("Map的Value必须是结构体类型");
			return true;
		}

		// 结构体成员检查（仅在非NoContainer模式下检查）
		if (ContainerType != EMapTreeContainerType::NoContainer)
		{
			if (const UScriptStruct* StructType = Cast<UScriptStruct>(OtherPin->PinType.PinValueType.TerminalSubCategoryObject.Get()))
			{
				// 新版本：检查结构体中是否至少有一个匹配类型的容器成员
				bool bHasValidMember = false;

				for (TFieldIterator<FProperty> PropIt(StructType); PropIt; ++PropIt)
				{
					FProperty* Property = *PropIt;
					bool bIsValidType = false;

					switch (ContainerType)
					{
						case EMapTreeContainerType::Array:
							bIsValidType = Property->IsA<FArrayProperty>();
							break;
						case EMapTreeContainerType::Map:
							bIsValidType = Property->IsA<FMapProperty>();
							break;
						case EMapTreeContainerType::Set:
							bIsValidType = Property->IsA<FSetProperty>();
							break;
						case EMapTreeContainerType::NoContainer:
							// NoContainer 模式不应该进入这个分支，但为了避免编译警告
							bIsValidType = false;
							break;
					}

					if (bIsValidType)
					{
						bHasValidMember = true;
						break;
					}
				}

				if (!bHasValidMember)
				{
					switch (ContainerType)
					{
						case EMapTreeContainerType::Array:
							OutReason = TEXT("结构体中必须包含至少一个Array类型的成员");
							break;
						case EMapTreeContainerType::Map:
							OutReason = TEXT("结构体中必须包含至少一个Map类型的成员");
							break;
						case EMapTreeContainerType::Set:
							OutReason = TEXT("结构体中必须包含至少一个Set类型的成员");
							break;
						case EMapTreeContainerType::NoContainer:
							// NoContainer 模式不应该进入这个分支
							OutReason = TEXT("未选择容器类型");
							break;
					}
					return true;
				}
			}
		}
		return false;
	}

	// 如果是 Key、SubKey 或 Value 引脚，检查 Map 是否已连接
	if (MyPin->PinName == InputKeyPinName() || MyPin->PinName == InputSubKeyPinName() || MyPin->PinName == InputItemPinName())
	{
		UEdGraphPin* MapPin = FindPin(InputMapPinName());
		// 确保 MapPin 存在
		if (!MapPin)
		{
			return true; // 如果 MapPin 不存在，禁止连接
		}

		// 检查 Map 是否已连接
		bool bMapConnected = MapPin->LinkedTo.Num() > 0;

		// 如果 Map 未连接，禁止连接 Key、SubKey 和 Value
		if (!bMapConnected)
		{
			OutReason = TEXT("必须先连接Map引脚");
			return true;
		}

		// 如果是 Key 引脚
		if (MyPin->PinName == InputKeyPinName())
		{
			// Map 已连接，检查类型匹配
			FEdGraphPinType KeyType = GetKeyPinType();
			if (KeyType.PinCategory != UEdGraphSchema_K2::PC_Wildcard &&
				KeyType.PinCategory != OtherPin->PinType.PinCategory)
			{
				OutReason = TEXT("Key类型不匹配");
				return true;
			}
		}

		// 如果是 SubKey 引脚
		if (MyPin->PinName == InputSubKeyPinName())
		{
			// Map 已连接，检查类型匹配
			FEdGraphPinType SubKeyType = GetSubKeyPinType();
			if (SubKeyType.PinCategory != UEdGraphSchema_K2::PC_Wildcard &&
				SubKeyType.PinCategory != OtherPin->PinType.PinCategory)
			{
				OutReason = TEXT("SubKey类型不匹配");
				return true;
			}
		}

		// 如果是 Value 引脚
		if (MyPin->PinName == InputItemPinName())
		{
			FEdGraphPinType ValueType = GetItemPinType();
			if (ValueType.PinCategory != UEdGraphSchema_K2::PC_Wildcard &&
				ValueType.PinCategory != OtherPin->PinType.PinCategory)
			{
				OutReason = TEXT("Value类型不匹配");
				return true;
			}
		}
	}

	return false;
}

//———————— Pin.Refresh																							————

FEdGraphPinType UMagnusNode_MapTreeAdd::GetKeyPinType() const
{
	FEdGraphPinType PinType;
	PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;

	// 获取Map引脚
	UEdGraphPin* MapPin = FindPin(InputMapPinName());
	if (MapPin && MapPin->LinkedTo.Num() > 0)
	{
		// 从连接的Map引脚获取类型信息
		const FEdGraphPinType& MapPinType = MapPin->LinkedTo[0]->PinType;
		if (MapPinType.ContainerType == EPinContainerType::Map)
		{
			// 直接使用Map的键类型（KeyPinType）来设置
			PinType.PinCategory = MapPinType.PinCategory;
			PinType.PinSubCategory = MapPinType.PinSubCategory;
			PinType.PinSubCategoryObject = MapPinType.PinSubCategoryObject;
			PinType.ContainerType = EPinContainerType::None;
		}
	}

	return PinType;
}

FEdGraphPinType UMagnusNode_MapTreeAdd::GetSubKeyPinType() const
{
	FEdGraphPinType PinType;
	PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;

	// 如果 MemberReference 有效，使用它来查找属性
	if (MemberReference.IsValid())
	{
		// 使用 StructMemberHelper 查找属性
		FProperty* Property = StructMemberHelper::FindPropertyBySinglePath(
			MemberReference.StructType,
			MemberReference.MemberPath
		);

		// 如果找到了且是Map属性，获取其Key类型
		if (Property && Property->IsA<FMapProperty>())
		{
			if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
			{
				const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
				Schema->ConvertPropertyToPinType(MapProperty->KeyProp, PinType);
			}
		}
	}

	return PinType;
}

FEdGraphPinType UMagnusNode_MapTreeAdd::GetItemPinType() const
{
	FEdGraphPinType PinType;
	PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;

	// 如果 MemberReference 有效，使用它来查找属性
	if (MemberReference.IsValid())
	{
		// 使用 StructMemberHelper 查找属性
		FProperty* Property = StructMemberHelper::FindPropertyBySinglePath(
			MemberReference.StructType,
			MemberReference.MemberPath
		);

		if (Property)
		{
			const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

			// 根据容器类型获取内部元素类型
			if (ContainerType == EMapTreeContainerType::Array && Property->IsA<FArrayProperty>())
			{
				if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
				{
					Schema->ConvertPropertyToPinType(ArrayProperty->Inner, PinType);
				}
			}
			else if (ContainerType == EMapTreeContainerType::Map && Property->IsA<FMapProperty>())
			{
				if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
				{
					Schema->ConvertPropertyToPinType(MapProperty->ValueProp, PinType);
				}
			}
			else if (ContainerType == EMapTreeContainerType::Set && Property->IsA<FSetProperty>())
			{
				if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
				{
					Schema->ConvertPropertyToPinType(SetProperty->ElementProp, PinType);
				}
			}
		}
	}

	return PinType;
}

void UMagnusNode_MapTreeAdd::PropagatePinType()
{
	UEdGraphPin* MapPin = FindPin(InputMapPinName());
	UEdGraphPin* KeyPin = FindPin(InputKeyPinName());
	UEdGraphPin* SubKeyPin = FindPin(InputSubKeyPinName());
	UEdGraphPin* ValuePin = FindPin(InputItemPinName());

	// 如果 Map 引脚有连接
	if (MapPin && MapPin->LinkedTo.Num() > 0)
	{
		const FEdGraphPinType& MapPinType = MapPin->LinkedTo[0]->PinType;
		if (MapPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			MapPin->PinType = MapPinType;
		}

		// 使用GetKeyPinType和GetItemPinType获取类型
		if (KeyPin && KeyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			KeyPin->PinType = GetKeyPinType();
		}

		if (SubKeyPin && SubKeyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			SubKeyPin->PinType = GetSubKeyPinType();
		}

		if (ValuePin && ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			ValuePin->PinType = GetItemPinType();
		}
	}
	else
	{
		// 重置为Wildcard类型
		if (MapPin)
		{
			MapPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			MapPin->PinType.ContainerType = EPinContainerType::Map;

			// 重要：重置Map的键值类型
			FEdGraphTerminalType WildcardTerminal;
			WildcardTerminal.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;
			MapPin->PinType.PinValueType = WildcardTerminal;

			// 清除可能残留的类型信息
			MapPin->PinType.PinSubCategoryObject = nullptr;
			MapPin->PinType.PinSubCategory = NAME_None;
		}

		if (KeyPin)
		{
			KeyPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			KeyPin->PinType.ContainerType = EPinContainerType::None;
			KeyPin->PinType.PinSubCategoryObject = nullptr;
			KeyPin->PinType.PinSubCategory = NAME_None;
		}

		if (SubKeyPin)
		{
			SubKeyPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			SubKeyPin->PinType.ContainerType = EPinContainerType::None;
			SubKeyPin->PinType.PinSubCategoryObject = nullptr;
			SubKeyPin->PinType.PinSubCategory = NAME_None;
		}

		if (ValuePin)
		{
			ValuePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			ValuePin->PinType.ContainerType = EPinContainerType::None;
			ValuePin->PinType.PinSubCategoryObject = nullptr;
			ValuePin->PinType.PinSubCategory = NAME_None;
		}
	}

	GetGraph()->NotifyGraphChanged();
}

//================ Container.Type.Management																========

//———————— Property.Change																						————

#if WITH_EDITOR
void UMagnusNode_MapTreeAdd::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		FName PropertyName = PropertyChangedEvent.Property->GetFName();

		// 获取顶层属性名（用于检测 MemberReference 的子属性变化）
		FName MemberPropertyName = NAME_None;
		if (PropertyChangedEvent.MemberProperty)
		{
			MemberPropertyName = PropertyChangedEvent.MemberProperty->GetFName();
		}

		// 如果修改的是ContainerType属性
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UMagnusNode_MapTreeAdd, ContainerType))
		{
			// 如果切换到NoContainer，需要断开非Map引脚
			if (ContainerType == EMapTreeContainerType::NoContainer)
			{
				if (UEdGraphPin* KeyPin = FindPin(InputKeyPinName()))
				{
					KeyPin->BreakAllPinLinks();
				}
				if (UEdGraphPin* SubKeyPin = FindPin(InputSubKeyPinName()))
				{
					SubKeyPin->BreakAllPinLinks();
				}
				if (UEdGraphPin* ValuePin = FindPin(InputItemPinName()))
				{
					ValuePin->BreakAllPinLinks();
				}
			}

			// 重置MemberReference（容器类型变了，之前的成员可能不适用）
			MemberReference.Reset();

			// 重建节点
			ReconstructNode();
		}

		// 如果修改的是MemberReference的子属性（MemberPath 或 StructType）
		// 或者顶层属性是 MemberReference
		bool bIsMemberReferenceChanged =
			(MemberPropertyName == GET_MEMBER_NAME_CHECKED(UMagnusNode_MapTreeAdd, MemberReference)) ||
			(PropertyName == GET_MEMBER_NAME_CHECKED(FStructMemberReference, MemberPath)) ||
			(PropertyName == GET_MEMBER_NAME_CHECKED(FStructMemberReference, StructType));

		if (bIsMemberReferenceChanged)
		{
			// 自动检测 ContainerType
			if (MemberReference.IsValid())
			{
				// 使用 StructMemberHelper 查找属性
				FProperty* Property = StructMemberHelper::FindPropertyBySinglePath(
					MemberReference.StructType,
					MemberReference.MemberPath
				);

				if (Property)
				{
					// 找到匹配的成员，自动检测其类型
					if (Property->IsA<FArrayProperty>())
					{
						ContainerType = EMapTreeContainerType::Array;
					}
					else if (Property->IsA<FMapProperty>())
					{
						ContainerType = EMapTreeContainerType::Map;
					}
					else if (Property->IsA<FSetProperty>())
					{
						ContainerType = EMapTreeContainerType::Set;
					}
					else
					{
						ContainerType = EMapTreeContainerType::NoContainer;
					}

					// 重建节点以应用新的ContainerType
					ReconstructNode();
				}
				else
				{
					// 安全检查：如果没有找到匹配的成员，设置为NoContainer
					ContainerType = EMapTreeContainerType::NoContainer;
					ReconstructNode();
				}
			}
			else
			{
				// MemberReference无效，重置为NoContainer
				if (ContainerType != EMapTreeContainerType::NoContainer)
				{
					ContainerType = EMapTreeContainerType::NoContainer;
					ReconstructNode();
				}
			}

			// 刷新引脚类型
			PropagatePinType();
			GetGraph()->NotifyGraphChanged();
		}
	}
}
#endif

//================ Blueprint.Integration																		========

//———————— Blueprint.Menu																							————

void UMagnusNode_MapTreeAdd::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

//———————— Blueprint.Compile																						————

HNCH_StartExpandNode(UMagnusNode_MapTreeAdd)

	virtual void Compile() override
	{
		// 安全检查：如果 MemberReference 无效，报错
		if (!OwnerNode->MemberReference.IsValid() || OwnerNode->ContainerType == EMapTreeContainerType::NoContainer)
		{
			CompilerContext.MessageLog.Error(
				*NSLOCTEXT("K2Node", "MapTreeAdd_InvalidMember", "MapTreeAdd: 请先选择一个有效的结构体成员！").ToString(),
				OwnerNode
			);
			// 仍然连接执行流，避免蓝图编译失败
			Link(ProxyExecPin(), ProxyThenPin());
			return;
		}

		// 根据容器类型执行完整的编译流程
		switch (OwnerNode->ContainerType)
		{

			case EMapTreeContainerType::Array:
			{
				// ========== Array 完整处理流程 ==========

				// 1. 创建 MapTree_AddArrayItem 函数节点
				UK2Node_CallFunction* CallNode = HNCH_SpawnFunctionNode(UMagnusFuncLib_MapTree, MapTree_AddArrayItem);

				// 2. 连接执行引脚
				Link(ProxyExecPin(), ExecPin(CallNode));
				Link(ThenPin(CallNode), ProxyThenPin());

				// 3. 连接数据引脚：TargetMap, Key, MemberName, Item
				Link(ProxyPin(OwnerNode->InputMapPinName()), FunctionInputPin(CallNode, "TargetMap"));
				Link(ProxyPin(OwnerNode->InputKeyPinName()), FunctionInputPin(CallNode, "Key"));

				// 设置 MemberName 默认值
				FString MemberNameValue = OwnerNode->MemberReference.MemberPath;
				if (!MemberNameValue.IsEmpty())
				{
					if (UEdGraphPin* MemberNamePin = FunctionInputPin(CallNode, "MemberName"))
					{
						MemberNamePin->DefaultValue = MemberNameValue;
					}
				}

				// Array 特定：连接 Item 参数
				Link(ProxyPin(OwnerNode->InputItemPinName()), FunctionInputPin(CallNode, "Item"));

				break;
			}

			case EMapTreeContainerType::Set:
			{
				// ========== Set 完整处理流程 ==========

				// 1. 创建 MapTree_AddSetItem 函数节点
				UK2Node_CallFunction* CallNode = HNCH_SpawnFunctionNode(UMagnusFuncLib_MapTree, MapTree_AddSetItem);

				// 2. 连接执行引脚
				Link(ProxyExecPin(), ExecPin(CallNode));
				Link(ThenPin(CallNode), ProxyThenPin());

				// 3. 连接数据引脚：TargetMap, Key, MemberName, Item
				Link(ProxyPin(OwnerNode->InputMapPinName()), FunctionInputPin(CallNode, "TargetMap"));
				Link(ProxyPin(OwnerNode->InputKeyPinName()), FunctionInputPin(CallNode, "Key"));

				// 设置 MemberName 默认值
				FString MemberNameValue = OwnerNode->MemberReference.MemberPath;
				if (!MemberNameValue.IsEmpty())
				{
					if (UEdGraphPin* MemberNamePin = FunctionInputPin(CallNode, "MemberName"))
					{
						MemberNamePin->DefaultValue = MemberNameValue;
					}
				}

				// Set 特定：连接 Item 参数
				Link(ProxyPin(OwnerNode->InputItemPinName()), FunctionInputPin(CallNode, "Item"));

				break;
			}

			case EMapTreeContainerType::Map:
			{
				// ========== Map 完整处理流程 ==========

				// 1. 创建 MapTree_AddMapItem 函数节点
				UK2Node_CallFunction* CallNode = HNCH_SpawnFunctionNode(UMagnusFuncLib_MapTree, MapTree_AddMapItem);

				// 2. 连接执行引脚
				Link(ProxyExecPin(), ExecPin(CallNode));
				Link(ThenPin(CallNode), ProxyThenPin());

				// 3. 连接数据引脚：TargetMap, Key, MemberName, SubKey, Value
				Link(ProxyPin(OwnerNode->InputMapPinName()), FunctionInputPin(CallNode, "TargetMap"));
				Link(ProxyPin(OwnerNode->InputKeyPinName()), FunctionInputPin(CallNode, "Key"));

				// 设置 MemberName 默认值
				FString MemberNameValue = OwnerNode->MemberReference.MemberPath;
				if (!MemberNameValue.IsEmpty())
				{
					if (UEdGraphPin* MemberNamePin = FunctionInputPin(CallNode, "MemberName"))
					{
						MemberNamePin->DefaultValue = MemberNameValue;
					}
				}

				// Map 特定：连接 SubKey 参数
				Link(ProxyPin(OwnerNode->InputSubKeyPinName()), FunctionInputPin(CallNode, "SubKey"));

				// Map 特定：最后一个参数叫 Value（不是 Item）
				Link(ProxyPin(OwnerNode->InputItemPinName()), FunctionInputPin(CallNode, "Value"));

				break;
			}

			case EMapTreeContainerType::NoContainer:
			default:
			{
				// 不应该到达这里，因为前面已经检查过了
				Link(ProxyExecPin(), ProxyThenPin());
				break;
			}
		}
	}

HNCH_EndExpandNode(UMagnusNode_MapTreeAdd)

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
