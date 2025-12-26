// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructViewerFilter.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SComboButton;
struct FStructMemberTreeNode;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// Generic Struct Picker Widget
class MAGNUSEXTENSION_API SStructPicker : public SCompoundWidget
{

public:

	//================ Delegates																				========

	// 当 Struct 被选择时的回调委托
	DECLARE_DELEGATE_OneParam(FOnStructPicked, const UScriptStruct* /*SelectedStruct*/);

	//================ Slate.Arguments																			========

	SLATE_BEGIN_ARGS(SStructPicker)
		: _InitiallySelectedStruct(nullptr)
		, _AllowNoneOption(false)
		, _ShowSearchBox(true)
		, _StructFilter(nullptr)
	{}
		// 初始选中的 Struct
		SLATE_ARGUMENT(const UScriptStruct*, InitiallySelectedStruct)

		// 是否允许选择 "None" 选项
		SLATE_ARGUMENT(bool, AllowNoneOption)

		// 是否显示搜索框
		SLATE_ARGUMENT(bool, ShowSearchBox)

		// 自定义 Struct 过滤器（可选，不指定则显示所有结构体）
		SLATE_ARGUMENT(TSharedPtr<IStructViewerFilter>, StructFilter)

		// 当 Struct 被选择时的回调
		SLATE_EVENT(FOnStructPicked, OnStructPicked)

	SLATE_END_ARGS()

	//================ Construction																				========

	// 构造函数
	SStructPicker();
	virtual ~SStructPicker();

	// 构造 Widget
	void Construct(const FArguments& InArgs);

	//================ Widget.Creation																			========

	// 创建内部的 Struct Viewer Widget
	TSharedRef<SWidget> CreateStructViewerWidget();

	// 创建使用引擎 StructViewer 模块的版本
	TSharedRef<SWidget> CreateUsingStructViewerModule();

	//================ Event.Handlers																			========

	// 处理 Struct 选择事件
	void OnStructSelected(const UScriptStruct* InStruct);

	//================ Properties.State																			========

	// 当前选中的 Struct
	const UScriptStruct* SelectedStruct;

	// Struct 选择回调
	FOnStructPicked OnStructPicked;

	// 是否允许 "None" 选项
	bool bAllowNoneOption;

	// 是否显示搜索框
	bool bShowSearchBox;

	// 自定义过滤器
	TSharedPtr<IStructViewerFilter> CustomStructFilter;

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// 通用 Struct 过滤器（不做任何限制，允许所有结构体）
// 用于 StructViewer 模块
class MAGNUSEXTENSION_API FGenericStructFilter : public IStructViewerFilter
{

public:

	//================ Filter.Override																			========

	// 检查指定的 Struct 是否应该被显示
	virtual bool IsStructAllowed(
		const FStructViewerInitializationOptions& InInitOptions,
		const UScriptStruct* InStruct,
		TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override;

	// 检查未加载的 Struct 是否应该被显示
	virtual bool IsUnloadedStructAllowed(
		const FStructViewerInitializationOptions& InInitOptions,
		const FSoftObjectPath& InStructPath,
		TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override;

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
