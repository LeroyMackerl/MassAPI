// Leroy Works & Ember, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "SGraphPin.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "StructViewerFilter.h"
#endif

class SComboButton;
class UScriptStruct;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if WITH_EDITOR

 // 自定义 Pin Widget，用于在蓝图节点中选择 Mass Fragment 类型
class MASSAPIUNCOOKED_API SGraphPinMassFragmentType : public SGraphPin
{
public:

	//================ Slate.Arguments																				========

	SLATE_BEGIN_ARGS(SGraphPinMassFragmentType) {}
	SLATE_END_ARGS()

	//================ Construction																					========

	// 构造 Widget
	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	//================ Widget.Creation																				========

protected:

	// 重写基类方法，创建自定义的值显示 Widget
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;

	//================ Event.Handlers																				========

	// 当用户点击下拉按钮时
	TSharedRef<SWidget> OnGetMenuContent();

	// 当 Fragment 被选择时
	void OnFragmentPicked(const UScriptStruct* SelectedFragment);

	// 获取当前选中的 Fragment 显示文本
	FText GetSelectedFragmentText() const;

	// 获取当前选中的 Fragment Tooltip
	FText GetSelectedFragmentTooltip() const;

	//================ Properties.UI																					========

	// 下拉按钮的引用
	TSharedPtr<SComboButton> ComboButton;

	// 获取当前选中的 Fragment
	const UScriptStruct* GetCurrentlySelectedFragment() const;

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// 自定义 Pin Widget，用于在蓝图节点中选择通用结构体类型（不限于 Mass Fragment）
class MASSAPIUNCOOKED_API SGraphPinStructType : public SGraphPin
{
public:

	//================ Slate.Arguments																				========

	SLATE_BEGIN_ARGS(SGraphPinStructType) {}
	SLATE_END_ARGS()

	//================ Construction																					========

	// 构造 Widget
	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	//================ Widget.Creation																				========

protected:

	// 重写基类方法，创建自定义的值显示 Widget
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;

	//================ Event.Handlers																				========

	// 当用户点击下拉按钮时
	TSharedRef<SWidget> OnGetMenuContent();

	// 当 Struct 被选择时
	void OnStructPicked(const UScriptStruct* SelectedStruct);

	// 获取当前选中的 Struct 显示文本
	FText GetSelectedStructText() const;

	// 获取当前选中的 Struct Tooltip
	FText GetSelectedStructTooltip() const;

	//================ Properties.UI																					========

	// 下拉按钮的引用
	TSharedPtr<SComboButton> ComboButton;

	// 获取当前选中的 Struct
	const UScriptStruct* GetCurrentlySelectedStruct() const;

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// Mass Fragment 过滤器
// 用于 StructViewer 模块，只显示 Mass Fragment 相关的类型
class MASSAPIUNCOOKED_API FMassFragmentStructFilter : public IStructViewerFilter
{

public:

	//================ Filter.Override																			========

	// 检查指定的 Fragment 是否应该被显示
	virtual bool IsStructAllowed(
		const FStructViewerInitializationOptions& InInitOptions,
		const UScriptStruct* InFragment,
		TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override;

	// 检查未加载的 Fragment 是否应该被显示
	virtual bool IsUnloadedStructAllowed(
		const FStructViewerInitializationOptions& InInitOptions,
		const FSoftObjectPath& InFragmentPath,
		TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override;

	//================ Filter.Utilities																			========

	// 检查是否是允许的 Mass Fragment 类型
	bool IsAllowedMassFragmentType(const UScriptStruct* InFragment) const;

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// 自定义 Pin Widget，用于在蓝图节点中选择 Mass Tag 类型
class MASSAPIUNCOOKED_API SGraphPinMassTagType : public SGraphPin
{
public:

	//================ Slate.Arguments																				========

	SLATE_BEGIN_ARGS(SGraphPinMassTagType) {}
	SLATE_END_ARGS()

	//================ Construction																					========

	// 构造 Widget
	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	//================ Widget.Creation																				========

protected:

	// 重写基类方法，创建自定义的值显示 Widget
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;

	//================ Event.Handlers																				========

	// 当用户点击下拉按钮时
	TSharedRef<SWidget> OnGetMenuContent();

	// 当 Tag 被选择时
	void OnTagPicked(const UScriptStruct* SelectedTag);

	// 获取当前选中的 Tag 显示文本
	FText GetSelectedTagText() const;

	// 获取当前选中的 Tag Tooltip
	FText GetSelectedTagTooltip() const;

	//================ Properties.UI																					========

	// 下拉按钮的引用
	TSharedPtr<SComboButton> ComboButton;

	// 获取当前选中的 Tag
	const UScriptStruct* GetCurrentlySelectedTag() const;

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// Mass Tag 过滤器
// 用于 StructViewer 模块，只显示 Mass Tag 相关的类型
class MASSAPIUNCOOKED_API FMassTagStructFilter : public IStructViewerFilter
{

public:

	//================ Filter.Override																			========

	// 检查指定的 Tag 是否应该被显示
	virtual bool IsStructAllowed(
		const FStructViewerInitializationOptions& InInitOptions,
		const UScriptStruct* InTag,
		TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override;

	// 检查未加载的 Tag 是否应该被显示
	virtual bool IsUnloadedStructAllowed(
		const FStructViewerInitializationOptions& InInitOptions,
		const FSoftObjectPath& InTagPath,
		TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override;

	//================ Filter.Utilities																			========

	// 检查是否是允许的 Mass Tag 类型
	bool IsAllowedMassTagType(const UScriptStruct* InTag) const;

};

#endif

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————