// Leroy Works & Ember, All Rights Reserved.

#include "Slate/SGraphPinMassAPI.h"

#if WITH_EDITOR
#include "Slate/SStructPicker.h"
#include "MassEntityTypes.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "ScopedTransaction.h"
#endif

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if WITH_EDITOR

//================ SGraphPinMassFragmentType																		========

//================ Construction																					========

void SGraphPinMassFragmentType::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	// 调用基类的 Construct
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

//================ Widget.Creation																				========

TSharedRef<SWidget> SGraphPinMassFragmentType::GetDefaultValueWidget()
{
	// 创建下拉按钮，用于显示和选择 Fragment
	return SAssignNew(ComboButton, SComboButton)
		.OnGetMenuContent(this, &SGraphPinMassFragmentType::OnGetMenuContent)
		.ContentPadding(FMargin(2.0f, 2.0f))
		.ButtonContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(1.0f)
			[
				SNew(STextBlock)
				.Text(this, &SGraphPinMassFragmentType::GetSelectedFragmentText)
				.ToolTipText(this, &SGraphPinMassFragmentType::GetSelectedFragmentTooltip)
				.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
		];
}

//================ Event.Handlers																				========

TSharedRef<SWidget> SGraphPinMassFragmentType::OnGetMenuContent()
{
	// 获取当前选中的 Fragment
	const UScriptStruct* CurrentFragment = GetCurrentlySelectedFragment();

	// 创建 Fragment Picker (使用 SStructPicker + 自定义过滤器)
	return SNew(SBox)
		.MinDesiredWidth(480.f)
		.MaxDesiredHeight(480.f)
		[
			SNew(SStructPicker)
			.InitiallySelectedStruct(CurrentFragment)
			.AllowNoneOption(true)
			.ShowSearchBox(true)
			.StructFilter(MakeShared<FMassFragmentStructFilter>())
			.OnStructPicked(this, &SGraphPinMassFragmentType::OnFragmentPicked)
		];
}

void SGraphPinMassFragmentType::OnFragmentPicked(const UScriptStruct* SelectedFragment)
{
	if (GraphPinObj)
	{
		// 创建事务以支持撤销/重做
		const FScopedTransaction Transaction(FText::FromString(TEXT("Change Fragment Type")));
		GraphPinObj->Modify();

		// 设置新的 Fragment 类型
		GraphPinObj->DefaultObject = const_cast<UScriptStruct*>(SelectedFragment);

		// 通知 Schema 值已改变
		if (const UEdGraphSchema* Schema = GraphPinObj->GetSchema())
		{
			Schema->TrySetDefaultObject(*GraphPinObj, const_cast<UScriptStruct*>(SelectedFragment));
		}

		// 触发引脚值改变事件
		if (GraphPinObj->GetOwningNode())
		{
			GraphPinObj->GetOwningNode()->PinDefaultValueChanged(GraphPinObj);
		}

		// 关闭下拉菜单
		if (ComboButton.IsValid())
		{
			ComboButton->SetIsOpen(false);
		}
	}
}

FText SGraphPinMassFragmentType::GetSelectedFragmentText() const
{
	const UScriptStruct* CurrentFragment = GetCurrentlySelectedFragment();

	if (CurrentFragment)
	{
		// 显示 Fragment 的名称
		return FText::FromString(CurrentFragment->GetName());
	}

	// 未选择时显示占位符
	return FText::FromString(TEXT("Select Fragment..."));
}

FText SGraphPinMassFragmentType::GetSelectedFragmentTooltip() const
{
	const UScriptStruct* CurrentFragment = GetCurrentlySelectedFragment();

	if (CurrentFragment)
	{
		// 显示完整的 Fragment 路径
		return FText::FromString(CurrentFragment->GetPathName());
	}

	return FText::FromString(TEXT("Click to select a Mass Fragment type"));
}

//================ Properties.UI																					========

const UScriptStruct* SGraphPinMassFragmentType::GetCurrentlySelectedFragment() const
{
	if (GraphPinObj && GraphPinObj->DefaultObject)
	{
		return Cast<UScriptStruct>(GraphPinObj->DefaultObject);
	}

	return nullptr;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ SGraphPinStructType																			========

//================ Construction																					========

void SGraphPinStructType::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	// 调用基类的 Construct
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

//================ Widget.Creation																				========

TSharedRef<SWidget> SGraphPinStructType::GetDefaultValueWidget()
{
	// 创建下拉按钮，用于显示和选择 Struct
	return SAssignNew(ComboButton, SComboButton)
		.OnGetMenuContent(this, &SGraphPinStructType::OnGetMenuContent)
		.ContentPadding(FMargin(2.0f, 2.0f))
		.ButtonContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(1.0f)
			[
				SNew(STextBlock)
				.Text(this, &SGraphPinStructType::GetSelectedStructText)
				.ToolTipText(this, &SGraphPinStructType::GetSelectedStructTooltip)
				.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
		];
}

//================ Event.Handlers																				========

TSharedRef<SWidget> SGraphPinStructType::OnGetMenuContent()
{
	// 获取当前选中的 Struct
	const UScriptStruct* CurrentStruct = GetCurrentlySelectedStruct();

	// 创建通用 Struct Picker
	return SNew(SBox)
		.MinDesiredWidth(480.f)
		.MaxDesiredHeight(480.f)
		[
			SNew(SStructPicker)
			.InitiallySelectedStruct(CurrentStruct)
			.AllowNoneOption(true)
			.ShowSearchBox(true)
			.OnStructPicked(this, &SGraphPinStructType::OnStructPicked)
		];
}

void SGraphPinStructType::OnStructPicked(const UScriptStruct* SelectedStruct)
{
	if (GraphPinObj)
	{
		// 创建事务以支持撤销/重做
		const FScopedTransaction Transaction(FText::FromString(TEXT("Change Struct Type")));
		GraphPinObj->Modify();

		// 设置新的 Struct 类型
		GraphPinObj->DefaultObject = const_cast<UScriptStruct*>(SelectedStruct);

		// 通知 Schema 值已改变
		if (const UEdGraphSchema* Schema = GraphPinObj->GetSchema())
		{
			Schema->TrySetDefaultObject(*GraphPinObj, const_cast<UScriptStruct*>(SelectedStruct));
		}

		// 触发引脚值改变事件
		if (GraphPinObj->GetOwningNode())
		{
			GraphPinObj->GetOwningNode()->PinDefaultValueChanged(GraphPinObj);
		}

		// 关闭下拉菜单
		if (ComboButton.IsValid())
		{
			ComboButton->SetIsOpen(false);
		}
	}
}

FText SGraphPinStructType::GetSelectedStructText() const
{
	const UScriptStruct* CurrentStruct = GetCurrentlySelectedStruct();

	if (CurrentStruct)
	{
		// 显示 Struct 的名称
		return FText::FromString(CurrentStruct->GetName());
	}

	// 未选择时显示占位符
	return FText::FromString(TEXT("Select Struct..."));
}

FText SGraphPinStructType::GetSelectedStructTooltip() const
{
	const UScriptStruct* CurrentStruct = GetCurrentlySelectedStruct();

	if (CurrentStruct)
	{
		// 显示完整的 Struct 路径
		return FText::FromString(CurrentStruct->GetPathName());
	}

	return FText::FromString(TEXT("Click to select a Struct type"));
}

//================ Properties.UI																					========

const UScriptStruct* SGraphPinStructType::GetCurrentlySelectedStruct() const
{
	if (GraphPinObj && GraphPinObj->DefaultObject)
	{
		return Cast<UScriptStruct>(GraphPinObj->DefaultObject);
	}

	return nullptr;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ FMassFragmentStructFilter																		========

bool FMassFragmentStructFilter::IsStructAllowed(
	const FStructViewerInitializationOptions& InInitOptions,
	const UScriptStruct* InFragment,
	TSharedRef<FStructViewerFilterFuncs> InFilterFuncs)
{
	if (!InFragment)
	{
		return false;
	}

	// 检查是否是允许的 Mass Fragment 类型
	return IsAllowedMassFragmentType(InFragment);
}

bool FMassFragmentStructFilter::IsUnloadedStructAllowed(
	const FStructViewerInitializationOptions& InInitOptions,
	const FSoftObjectPath& InFragmentPath,
	TSharedRef<FStructViewerFilterFuncs> InFilterFuncs)
{
	// 未加载的结构体都是用户自定义结构体（User Defined Structs）
	// 用户自定义结构体不支持继承，因此永远不可能是 Mass Fragment 的子类
	// 所有的 Mass Fragment 都是 C++ 原生结构体，在启动时就会被加载
	// 因此这里直接返回 false，过滤掉所有未加载的结构体
	return false;
}

//================ Filter.Utilities																				========

bool FMassFragmentStructFilter::IsAllowedMassFragmentType(const UScriptStruct* InFragment) const
{
	if (!InFragment)
	{
		return false;
	}

	// 获取 Mass Fragment 基类型
	static const UScriptStruct* MassFragmentStruct = FMassFragment::StaticStruct();
	static const UScriptStruct* MassChunkFragmentStruct = FMassChunkFragment::StaticStruct();
	static const UScriptStruct* MassSharedFragmentStruct = FMassSharedFragment::StaticStruct();
	static const UScriptStruct* MassConstSharedFragmentStruct = FMassConstSharedFragment::StaticStruct();

	// 检查是否继承自这 4 种 Fragment 类型之一
	const bool bIsMassFragment = InFragment->IsChildOf(MassFragmentStruct);
	const bool bIsChunkFragment = InFragment->IsChildOf(MassChunkFragmentStruct);
	const bool bIsSharedFragment = InFragment->IsChildOf(MassSharedFragmentStruct);
	const bool bIsConstSharedFragment = InFragment->IsChildOf(MassConstSharedFragmentStruct);

	return bIsMassFragment || bIsChunkFragment || bIsSharedFragment || bIsConstSharedFragment;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ SGraphPinMassTagType																				========

//================ Construction																					========

void SGraphPinMassTagType::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	// 调用基类的 Construct
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

//================ Widget.Creation																				========

TSharedRef<SWidget> SGraphPinMassTagType::GetDefaultValueWidget()
{
	// 创建下拉按钮，用于显示和选择 Tag
	return SAssignNew(ComboButton, SComboButton)
		.OnGetMenuContent(this, &SGraphPinMassTagType::OnGetMenuContent)
		.ContentPadding(FMargin(2.0f, 2.0f))
		.ButtonContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(1.0f)
			[
				SNew(STextBlock)
				.Text(this, &SGraphPinMassTagType::GetSelectedTagText)
				.ToolTipText(this, &SGraphPinMassTagType::GetSelectedTagTooltip)
				.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
		];
}

//================ Event.Handlers																				========

TSharedRef<SWidget> SGraphPinMassTagType::OnGetMenuContent()
{
	// 获取当前选中的 Tag
	const UScriptStruct* CurrentTag = GetCurrentlySelectedTag();

	// 创建 Tag Picker (使用 SStructPicker + 自定义过滤器)
	return SNew(SBox)
		.MinDesiredWidth(480.f)
		.MaxDesiredHeight(480.f)
		[
			SNew(SStructPicker)
			.InitiallySelectedStruct(CurrentTag)
			.AllowNoneOption(true)
			.ShowSearchBox(true)
			.StructFilter(MakeShared<FMassTagStructFilter>())
			.OnStructPicked(this, &SGraphPinMassTagType::OnTagPicked)
		];
}

void SGraphPinMassTagType::OnTagPicked(const UScriptStruct* SelectedTag)
{
	if (GraphPinObj)
	{
		// 创建事务以支持撤销/重做
		const FScopedTransaction Transaction(FText::FromString(TEXT("Change Tag Type")));
		GraphPinObj->Modify();

		// 设置新的 Tag 类型
		GraphPinObj->DefaultObject = const_cast<UScriptStruct*>(SelectedTag);

		// 通知 Schema 值已改变
		if (const UEdGraphSchema* Schema = GraphPinObj->GetSchema())
		{
			Schema->TrySetDefaultObject(*GraphPinObj, const_cast<UScriptStruct*>(SelectedTag));
		}

		// 触发引脚值改变事件
		if (GraphPinObj->GetOwningNode())
		{
			GraphPinObj->GetOwningNode()->PinDefaultValueChanged(GraphPinObj);
		}

		// 关闭下拉菜单
		if (ComboButton.IsValid())
		{
			ComboButton->SetIsOpen(false);
		}
	}
}

FText SGraphPinMassTagType::GetSelectedTagText() const
{
	const UScriptStruct* CurrentTag = GetCurrentlySelectedTag();

	if (CurrentTag)
	{
		// 显示 Tag 的名称
		return FText::FromString(CurrentTag->GetName());
	}

	// 未选择时显示占位符
	return FText::FromString(TEXT("Select Tag..."));
}

FText SGraphPinMassTagType::GetSelectedTagTooltip() const
{
	const UScriptStruct* CurrentTag = GetCurrentlySelectedTag();

	if (CurrentTag)
	{
		// 显示完整的 Tag 路径
		return FText::FromString(CurrentTag->GetPathName());
	}

	return FText::FromString(TEXT("Click to select a Mass Tag type"));
}

//================ Properties.UI																					========

const UScriptStruct* SGraphPinMassTagType::GetCurrentlySelectedTag() const
{
	if (GraphPinObj && GraphPinObj->DefaultObject)
	{
		return Cast<UScriptStruct>(GraphPinObj->DefaultObject);
	}

	return nullptr;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ FMassTagStructFilter																			========

bool FMassTagStructFilter::IsStructAllowed(
	const FStructViewerInitializationOptions& InInitOptions,
	const UScriptStruct* InTag,
	TSharedRef<FStructViewerFilterFuncs> InFilterFuncs)
{
	if (!InTag)
	{
		return false;
	}

	// 检查是否是允许的 Mass Tag 类型
	return IsAllowedMassTagType(InTag);
}

bool FMassTagStructFilter::IsUnloadedStructAllowed(
	const FStructViewerInitializationOptions& InInitOptions,
	const FSoftObjectPath& InTagPath,
	TSharedRef<FStructViewerFilterFuncs> InFilterFuncs)
{
	// 未加载的结构体都是用户自定义结构体（User Defined Structs）
	// 用户自定义结构体不支持继承，因此永远不可能是 Mass Tag 的子类
	// 所有的 Mass Tag 都是 C++ 原生结构体，在启动时就会被加载
	// 因此这里直接返回 false，过滤掉所有未加载的结构体
	return false;
}

//================ Filter.Utilities																				========

bool FMassTagStructFilter::IsAllowedMassTagType(const UScriptStruct* InTag) const
{
	if (!InTag)
	{
		return false;
	}

	// 获取 Mass Tag 基类型
	static const UScriptStruct* MassTagStruct = FMassTag::StaticStruct();

	// 检查是否继承自 FMassTag
	return InTag->IsChildOf(MassTagStruct);
}

#endif // WITH_EDITOR

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
