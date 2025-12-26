#include "Property/MagnusNodesStruct.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if WITH_EDITOR
BEGIN_MCPH_IMPLEMENT(FStructRecursiveMemberReference)

MCPH_HEADER_IMPLEMENT
{
	// 获取属性句柄
	StructTypeHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FStructRecursiveMemberReference, StructType));
	MemberPathsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FStructRecursiveMemberReference, MemberPaths));

	// 获取当前结构体类型
	if (StructTypeHandle.IsValid())
	{
		UObject* StructObject = nullptr;
		StructTypeHandle->GetValue(StructObject);
		CurrentStructType = Cast<UScriptStruct>(StructObject);

		// 监听 StructType 变化
		StructTypeHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &MCPH_THIS_CLASS::OnStructTypeChanged));
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
	// 递归构建成员树
	BuildRecursiveMemberRows(ChildBuilder, CurrentStructType.Get(), TArray<FString>());
}

void MCPH_THIS_CLASS::BuildRecursiveMemberRows(
	IDetailChildrenBuilder& ChildBuilder,
	const UScriptStruct* StructType,
	const TArray<FString>& ParentPath)
{
	if (!StructType)
		return;

	// 遍历所有成员
	for (TFieldIterator<FProperty> It(StructType); It; ++It)
	{
		FProperty* Property = *It;
		if (!Property)
			continue;

		FString PropertyName = Property->GetName();
		FString PropertyType = Property->GetCPPType();
		FText PropertyDisplayName = Property->GetDisplayNameText();

		// 构建完整路径
		TArray<FString> CurrentPath = ParentPath;
		CurrentPath.Add(PropertyName);
		FString FullPath = FString::Join(CurrentPath, TEXT("."));

		// 检查是否是结构体类型
		FStructProperty* StructProperty = CastField<FStructProperty>(Property);
		bool bIsStruct = (StructProperty != nullptr && StructProperty->Struct != nullptr);

		if (bIsStruct)
		{
			// 如果是结构体，创建一个Group
			IDetailGroup& Group = ChildBuilder.AddGroup(FName(*FullPath), PropertyDisplayName);
			Group.ToggleExpansion(true); // 默认展开

			// 添加CheckBox行
			Group.HeaderRow()
				.NameContent()
				[
					SNew(SHorizontalBox)
					// CheckBox
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this, FullPath]()
						{
							return IsPathSelected(FullPath) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						})
						.OnCheckStateChanged_Lambda([this, FullPath](ECheckBoxState NewState)
						{
							TogglePathSelection(FullPath);
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

			// 递归构建子成员
			BuildRecursiveMemberRowsForGroup(Group, StructProperty->Struct, CurrentPath);
		}
		else
		{
			// 如果不是结构体，直接创建CheckBox行
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
						.IsChecked_Lambda([this, FullPath]()
						{
							return IsPathSelected(FullPath) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						})
						.OnCheckStateChanged_Lambda([this, FullPath](ECheckBoxState NewState)
						{
							TogglePathSelection(FullPath);
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
}

void MCPH_THIS_CLASS::BuildRecursiveMemberRowsForGroup(
	IDetailGroup& Group,
	const UScriptStruct* StructType,
	const TArray<FString>& ParentPath)
{
	if (!StructType)
		return;

	// 遍历所有子成员
	for (TFieldIterator<FProperty> It(StructType); It; ++It)
	{
		FProperty* Property = *It;
		if (!Property)
			continue;

		FString PropertyName = Property->GetName();
		FString PropertyType = Property->GetCPPType();
		FText PropertyDisplayName = Property->GetDisplayNameText();

		// 构建完整路径
		TArray<FString> CurrentPath = ParentPath;
		CurrentPath.Add(PropertyName);
		FString FullPath = FString::Join(CurrentPath, TEXT("."));

		// 检查是否是结构体类型
		FStructProperty* StructProperty = CastField<FStructProperty>(Property);
		bool bIsStruct = (StructProperty != nullptr && StructProperty->Struct != nullptr);

		if (bIsStruct)
		{
			// 如果是结构体，创建一个嵌套Group
			IDetailGroup& NestedGroup = Group.AddGroup(FName(*FullPath), PropertyDisplayName);
			NestedGroup.ToggleExpansion(true); // 默认展开

			// 添加CheckBox行
			NestedGroup.HeaderRow()
				.NameContent()
				[
					SNew(SHorizontalBox)
					// CheckBox
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this, FullPath]()
						{
							return IsPathSelected(FullPath) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						})
						.OnCheckStateChanged_Lambda([this, FullPath](ECheckBoxState NewState)
						{
							TogglePathSelection(FullPath);
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

			// 递归构建子成员
			BuildRecursiveMemberRowsForGroup(NestedGroup, StructProperty->Struct, CurrentPath);
		}
		else
		{
			// 如果不是结构体，直接在Group中添加CheckBox行
			Group.AddWidgetRow()
				.NameContent()
				[
					SNew(SHorizontalBox)
					// CheckBox
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this, FullPath]()
						{
							return IsPathSelected(FullPath) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						})
						.OnCheckStateChanged_Lambda([this, FullPath](ECheckBoxState NewState)
						{
							TogglePathSelection(FullPath);
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
	if (MemberPathsHandle.IsValid())
	{
		TArray<void*> RawData;
		MemberPathsHandle->AccessRawData(RawData);

		for (void* Data : RawData)
		{
			if (Data)
			{
				TArray<FString>* PathsArray = static_cast<TArray<FString>*>(Data);
				PathsArray->Empty();
			}
		}

		MemberPathsHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

bool MCPH_THIS_CLASS::IsPathSelected(const FString& Path) const
{
	if (!MemberPathsHandle.IsValid())
		return false;

	TArray<void*> RawData;
	MemberPathsHandle->AccessRawData(RawData);

	for (void* Data : RawData)
	{
		if (Data)
		{
			TArray<FString>* PathsArray = static_cast<TArray<FString>*>(Data);
			if (PathsArray->Contains(Path))
				return true;
		}
	}

	return false;
}

bool MCPH_THIS_CLASS::IsParentOf(const FString& ParentPath, const FString& ChildPath) const
{
	// 检查ChildPath是否以"ParentPath."开头
	return ChildPath.StartsWith(ParentPath + TEXT("."));
}

bool MCPH_THIS_CLASS::IsChildOf(const FString& ChildPath, const FString& ParentPath) const
{
	return IsParentOf(ParentPath, ChildPath);
}

void MCPH_THIS_CLASS::RemoveConflictingPaths(const FString& Path, TArray<FString>& Paths) const
{
	// 移除所有是Path的父节点或子节点的路径
	Paths.RemoveAll([this, &Path](const FString& ExistingPath)
	{
		// 如果ExistingPath是Path的父节点或子节点，则移除
		return IsParentOf(ExistingPath, Path) || IsChildOf(ExistingPath, Path);
	});
}

void MCPH_THIS_CLASS::TogglePathSelection(const FString& Path)
{
	if (!MemberPathsHandle.IsValid())
		return;

	TArray<void*> RawData;
	MemberPathsHandle->AccessRawData(RawData);

	for (void* Data : RawData)
	{
		if (Data)
		{
			TArray<FString>* PathsArray = static_cast<TArray<FString>*>(Data);

			// 检查Path是否已经被选中
			bool bWasSelected = PathsArray->Contains(Path);

			if (bWasSelected)
			{
				// 取消选中：直接移除
				PathsArray->Remove(Path);
			}
			else
			{
				// 选中：先移除所有冲突的路径，然后添加
				RemoveConflictingPaths(Path, *PathsArray);
				PathsArray->Add(Path);
			}
		}
	}

	// 通知属性已更改
	MemberPathsHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
}

FText MCPH_THIS_CLASS::GetCurrentValueText() const
{
	if (!CurrentStructType.IsValid())
	{
		return NSLOCTEXT("StructRecursiveMemberRef", "NoStruct", "No Struct Selected");
	}

	if (!MemberPathsHandle.IsValid())
	{
		return FText::Format(
			NSLOCTEXT("StructRecursiveMemberRef", "StructOnly", "{0} :: (No Member)"),
			CurrentStructType->GetDisplayNameText()
		);
	}

	// 获取当前选中的路径数量
	TArray<void*> RawData;
	MemberPathsHandle->AccessRawData(RawData);

	int32 TotalCount = 0;
	for (void* Data : RawData)
	{
		if (Data)
		{
			TArray<FString>* PathsArray = static_cast<TArray<FString>*>(Data);
			TotalCount += PathsArray->Num();
		}
	}

	if (TotalCount == 0)
	{
		return FText::Format(
			NSLOCTEXT("StructRecursiveMemberRef", "StructOnly", "{0} :: (No Member)"),
			CurrentStructType->GetDisplayNameText()
		);
	}

	if (TotalCount == 1)
	{
		// 显示单个路径
		for (void* Data : RawData)
		{
			if (Data)
			{
				TArray<FString>* PathsArray = static_cast<TArray<FString>*>(Data);
				if (PathsArray->Num() > 0)
				{
					return FText::Format(
						NSLOCTEXT("StructRecursiveMemberRef", "Single", "{0} :: {1}"),
						CurrentStructType->GetDisplayNameText(),
						FText::FromString((*PathsArray)[0])
					);
				}
			}
		}
	}

	// 显示多个路径
	return FText::Format(
		NSLOCTEXT("StructRecursiveMemberRef", "Multiple", "{0} :: [{1} members]"),
		CurrentStructType->GetDisplayNameText(),
		FText::AsNumber(TotalCount)
	);
}

FReply MCPH_THIS_CLASS::OnClearClicked()
{
	// 清空所有值
	if (StructTypeHandle.IsValid())
	{
		StructTypeHandle->SetValue((UObject*)nullptr);
	}

	if (MemberPathsHandle.IsValid())
	{
		TArray<void*> RawData;
		MemberPathsHandle->AccessRawData(RawData);

		for (void* Data : RawData)
		{
			if (Data)
			{
				TArray<FString>* PathsArray = static_cast<TArray<FString>*>(Data);
				PathsArray->Empty();
			}
		}

		MemberPathsHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}

	CurrentStructType = nullptr;

	return FReply::Handled();
}

END_MCPH_IMPLEMENT
#endif

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
