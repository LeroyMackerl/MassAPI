// Copyright Epic Games, Inc. All Rights Reserved.

#include "Slate/SStructPicker.h"
#include "StructViewerModule.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Views/STableRow.h"
#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Construction																					========

SStructPicker::SStructPicker()
	: SelectedStruct(nullptr)
	, bAllowNoneOption(false)
	, bShowSearchBox(true)
{
}

SStructPicker::~SStructPicker()
{
}

void SStructPicker::Construct(const FArguments& InArgs)
{
	SelectedStruct = InArgs._InitiallySelectedStruct;
	OnStructPicked = InArgs._OnStructPicked;
	bAllowNoneOption = InArgs._AllowNoneOption;
	bShowSearchBox = InArgs._ShowSearchBox;
	CustomStructFilter = InArgs._StructFilter;

	// 使用引擎的 StructViewer 模块创建 Widget
	ChildSlot
	[
		CreateUsingStructViewerModule()
	];
}

//================ Widget.Creation																				========

TSharedRef<SWidget> SStructPicker::CreateStructViewerWidget()
{
	// 这个方法保留用于未来可能的自定义实现
	return CreateUsingStructViewerModule();
}

TSharedRef<SWidget> SStructPicker::CreateUsingStructViewerModule()
{
	// 配置 StructViewer 初始化选项
	FStructViewerInitializationOptions Options;

	// 设置为 Picker 模式（单选）
	Options.Mode = EStructViewerMode::StructPicker;

	// 使用树形视图显示继承层级
	Options.DisplayMode = EStructViewerDisplayMode::TreeView;

	// 显示未加载的 Struct
	Options.bShowUnloadedStructs = true;

	// 是否显示 "None" 选项
	Options.bShowNoneOption = bAllowNoneOption;

	// 默认折叠根节点
	Options.bExpandRootNodes = false;

	// 允许动态加载 Struct
	Options.bEnableStructDynamicLoading = true;

	// 显示 Struct 名称
	Options.NameTypeToDisplay = EStructViewerNameTypeToDisplay::StructName;

	// 设置初始选中的 Struct
	Options.SelectedStruct = SelectedStruct;

	// 显示视图选项
	Options.bAllowViewOptions = true;

	// 显示背景边框
	Options.bShowBackgroundBorder = true;

	// 如果用户提供了自定义过滤器就使用它，否则使用默认的通用过滤器
	Options.StructFilter = CustomStructFilter.IsValid()
		? CustomStructFilter
		: MakeShared<FGenericStructFilter>();

	// 加载 StructViewer 模块
	FStructViewerModule& StructViewerModule = FModuleManager::LoadModuleChecked<FStructViewerModule>("StructViewer");

	// 创建 StructViewer Widget
	return StructViewerModule.CreateStructViewer(
		Options,
		FOnStructPicked::CreateSP(this, &SStructPicker::OnStructSelected)
	);
}

//================ Event.Handlers																				========

void SStructPicker::OnStructSelected(const UScriptStruct* InStruct)
{
	SelectedStruct = InStruct;

	// 调用用户提供的回调
	if (OnStructPicked.IsBound())
		OnStructPicked.Execute(InStruct);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ Filter.Override																				========

bool FGenericStructFilter::IsStructAllowed(
	const FStructViewerInitializationOptions& InInitOptions,
	const UScriptStruct* InStruct,
	TSharedRef<FStructViewerFilterFuncs> InFilterFuncs)
{
	// 允许所有结构体
	return InStruct != nullptr;
}

bool FGenericStructFilter::IsUnloadedStructAllowed(
	const FStructViewerInitializationOptions& InInitOptions,
	const FSoftObjectPath& InStructPath,
	TSharedRef<FStructViewerFilterFuncs> InFilterFuncs)
{
	// 允许所有未加载的结构体（包括用户自定义结构体）
	return true;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
