#include "Property/MagnusNodesStruct.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if WITH_EDITOR

//====== FStructMemberReference 自定义面板实现 ======

BEGIN_MCPH_IMPLEMENT(FStructMemberReference)

MCPH_HEADER_IMPLEMENT
{
	// 保存主属性句柄
	this->MainPropertyHandle = PropertyHandle;

	// 获取子属性句柄
	StructTypeHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FStructMemberReference, StructType));
	MemberPathHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FStructMemberReference, MemberPath));

	// 从 Meta 加载过滤器
	LoadFilterFromMeta();

	// 获取当前结构体类型
	if (StructTypeHandle.IsValid())
	{
		UObject* StructObject = nullptr;
		StructTypeHandle->GetValue(StructObject);
		CurrentStructType = Cast<UScriptStruct>(StructObject);

		// 监听 StructType 变化
		StructTypeHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &MCPH_THIS_CLASS::OnStructTypeChanged)
		);
	}

	/*// 构建 Header
	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(300.0f)
		.MaxDesiredWidth(600.0f)
		[
			SNew(SHorizontalBox)
			// 显示当前值
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				SNew(STextBlock)
				.Text(this, &MCPH_THIS_CLASS::GetCurrentValueText)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];*/
}

MCPH_CHILDREN_IMPLEMENT
{
	// 只构建第一层成员（应用过滤）
	BuildFirstLevelMemberRows(ChildBuilder);
}

void MCPH_THIS_CLASS::BuildFirstLevelMemberRows(IDetailChildrenBuilder& ChildBuilder)
{
	// 每次重新读取结构体类型（确保获取最新值）
	UScriptStruct* StructType = nullptr;
	if (StructTypeHandle.IsValid())
	{
		UObject* StructObject = nullptr;
		StructTypeHandle->GetValue(StructObject);
		StructType = Cast<UScriptStruct>(StructObject);

		// 同步更新缓存
		CurrentStructType = StructType;
	}

	if (!StructType)
		return;

	// 只遍历第一层成员
	for (TFieldIterator<FProperty> It(StructType); It; ++It)
	{
		FProperty* Property = *It;
		if (!Property)
			continue;

		// ====== 应用过滤器 ======
		if (!ShouldShowProperty(Property))
			continue;

		FString PropertyName = Property->GetName();
		FString PropertyType = Property->GetCPPType();
		FText PropertyDisplayName = Property->GetDisplayNameText();

		// 创建单选按钮行
		ChildBuilder.AddCustomRow(PropertyDisplayName)
			.NameContent()
			[
				SNew(SHorizontalBox)
				// CheckBox
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this, PropertyName]()
					{
						return IsPathSelected(PropertyName)
							? ECheckBoxState::Checked
							: ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this, PropertyName](ECheckBoxState NewState)
					{
						if (NewState == ECheckBoxState::Checked)
						{
							SetSelectedPath(PropertyName);
						}
					})
				]
				// 成员名称
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.Padding(FMargin(5, 0))
				[
					SNew(STextBlock)
					.Text(PropertyDisplayName)
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
			]
			.ValueContent()
			[
				// 类型信息
				SNew(STextBlock)
				.Text(FText::FromString(PropertyType))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
			];
	}
}

void MCPH_THIS_CLASS::OnStructTypeChanged()
{
	// 更新当前结构体类型
	if (StructTypeHandle.IsValid())
	{
		UObject* StructObject = nullptr;
		StructTypeHandle->GetValue(StructObject);
		CurrentStructType = Cast<UScriptStruct>(StructObject);
	}

	// 清空成员路径
	if (MemberPathHandle.IsValid())
	{
		MemberPathHandle->SetValue(FString());
	}
}

bool MCPH_THIS_CLASS::IsPathSelected(const FString& Path) const
{
	if (!MemberPathHandle.IsValid())
		return false;

	FString CurrentPath;
	MemberPathHandle->GetValue(CurrentPath);
	return CurrentPath == Path;
}

void MCPH_THIS_CLASS::SetSelectedPath(const FString& Path)
{
	if (!MemberPathHandle.IsValid())
		return;

	MemberPathHandle->SetValue(Path);
}

FText MCPH_THIS_CLASS::GetCurrentValueText() const
{
	if (!CurrentStructType.IsValid())
	{
		return NSLOCTEXT("StructMemberRef", "NoStruct", "No Struct Selected");
	}

	FString CurrentPath;
	if (MemberPathHandle.IsValid())
	{
		MemberPathHandle->GetValue(CurrentPath);
	}

	if (CurrentPath.IsEmpty())
	{
		return FText::Format(
			NSLOCTEXT("StructMemberRef", "StructOnly", "{0} :: (No Member)"),
			CurrentStructType->GetDisplayNameText()
		);
	}

	return FText::Format(
		NSLOCTEXT("StructMemberRef", "Single", "{0} :: {1}"),
		CurrentStructType->GetDisplayNameText(),
		FText::FromString(CurrentPath)
	);
}

//====== 过滤相关实现 ======

void MCPH_THIS_CLASS::LoadFilterFromMeta()
{
	CurrentFilterFunc = nullptr;

	if (!MainPropertyHandle.IsValid())
		return;

	// 读取 meta 标签 "PropertyFilter"
	const FString& FilterString = MainPropertyHandle->GetMetaData(TEXT("PropertyFilter"));

	if (FilterString.IsEmpty())
	{
		// 无过滤器，显示所有属性
		return;
	}

	// 内置过滤器快捷方式
	if (FilterString.Equals(TEXT("ContainerOnly"), ESearchCase::IgnoreCase))
	{
		CurrentFilterFunc = [](const FProperty* Property) -> bool
		{
			return IsContainerProperty(Property);
		};
		return;
	}

	if (FilterString.Equals(TEXT("SingleOnly"), ESearchCase::IgnoreCase))
	{
		CurrentFilterFunc = [](const FProperty* Property) -> bool
		{
			return IsSingleProperty(Property);
		};
		return;
	}

	if (FilterString.Equals(TEXT("NumericOnly"), ESearchCase::IgnoreCase))
	{
		CurrentFilterFunc = [](const FProperty* Property) -> bool
		{
			return IsNumericProperty(Property);
		};
		return;
	}

	// 从全局注册表查找自定义过滤器
	FName FilterName(*FilterString);
	if (FPropertyFilterFunc* RegisteredFilter = FStructMemberPropertyFilterRegistry::GetFilter(FilterName))
	{
		CurrentFilterFunc = *RegisteredFilter;
	}
	else
	{
		// 过滤器未找到，输出警告
		UE_LOG(LogTemp, Warning, TEXT("FStructMemberReference: Property filter '%s' not found. Showing all properties."), *FilterString);
	}
}

bool MCPH_THIS_CLASS::ShouldShowProperty(const FProperty* Property) const
{
	if (!Property)
		return false;

	// 如果没有过滤器，显示所有属性
	if (!CurrentFilterFunc)
		return true;

	// 应用过滤器
	return CurrentFilterFunc(Property);
}

//====== 内置过滤器辅助函数 ======

bool MCPH_THIS_CLASS::IsContainerProperty(const FProperty* Property)
{
	if (!Property)
		return false;

	// 检查是否是容器类型
	return Property->IsA<FArrayProperty>() ||
	       Property->IsA<FSetProperty>() ||
	       Property->IsA<FMapProperty>();
}

bool MCPH_THIS_CLASS::IsSingleProperty(const FProperty* Property)
{
	if (!Property)
		return false;

	// 不是容器类型就是单一类型
	return !IsContainerProperty(Property);
}

bool MCPH_THIS_CLASS::IsNumericProperty(const FProperty* Property)
{
	if (!Property)
		return false;

	// 检查是否是数值类型
	return Property->IsA<FIntProperty>() ||
	       Property->IsA<FInt64Property>() ||
	       Property->IsA<FUInt32Property>() ||
	       Property->IsA<FUInt64Property>() ||
	       Property->IsA<FFloatProperty>() ||
	       Property->IsA<FDoubleProperty>() ||
	       Property->IsA<FByteProperty>() ||
	       Property->IsA<FInt8Property>() ||
	       Property->IsA<FInt16Property>() ||
	       Property->IsA<FUInt16Property>();
}

END_MCPH_IMPLEMENT

#endif // WITH_EDITOR

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
