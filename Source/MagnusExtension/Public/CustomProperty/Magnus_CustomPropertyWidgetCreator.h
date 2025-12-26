#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"

// Widget 相关头文件
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Styling/SlateColor.h"
#include "Styling/AppStyle.h"
#include "Framework/SlateDelegates.h"
#include "Misc/Attribute.h"

#include "Kismet/KismetMathLibrary.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//———————— Widget Creator						    																        ————

// 通用的动态网格UI模板函数
inline TSharedRef<SWidget> MCWH_ComboValueOptions(
	const TArray<TPair<TFunction<TSharedRef<SWidget>(int32)>, float>>& ColumnDefinitions,
	int32 PropertyRow = 2,
	float MinWidgetHeight = 0.f,
	float ComboSpacerWidth = 15.f,
	float EdgeSpacerHeight = 5.f)
{
	// 创建水平盒容器
	TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

	// 遍历每列定义
	for (int32 ColIndex = 0; ColIndex < ColumnDefinitions.Num(); ++ColIndex)
	{
		const TPair<TFunction<TSharedRef<SWidget>(int32)>, float>& ColumnDef = ColumnDefinitions[ColIndex];
		const TFunction<TSharedRef<SWidget>(int32)>& GetWidgetForRow = ColumnDef.Key;
		const float ColumnWidth = ColumnDef.Value;

		// 如果不是第一列，添加间距
		if (ColIndex > 0)
		{
			HorizontalBox->AddSlot()
			.AutoWidth()
			[
				SNew(SSpacer)
				.Size(FVector2D(ComboSpacerWidth, 0.f))
			];
		}

		// 创建当前列的垂直盒
		TSharedRef<SVerticalBox> ColumnVerticalBox = SNew(SVerticalBox);
		for (int32 RowIndex = 0; RowIndex < PropertyRow; ++RowIndex)
		{
			ColumnVerticalBox->AddSlot()
			.FillHeight(1.f)
			.VAlign(VAlign_Center)
			[
				GetWidgetForRow(RowIndex)
			];
		}

		// 添加列到水平盒
		HorizontalBox->AddSlot()
		.AutoWidth()
		[
			SNew(SBox)
			.MinDesiredWidth(ColumnWidth)
			[
				ColumnVerticalBox
			]
		];
	}

	// 如果不需要上下边距且没有最小高度要求，直接返回水平盒
	if (EdgeSpacerHeight <= 0.f && MinWidgetHeight <= 0.f)
	{
		return HorizontalBox;
	}

	// 创建最终内容控件
	TSharedRef<SWidget> ContentWidget = HorizontalBox;

	// 如果需要上下边距，包装垂直盒
	if (EdgeSpacerHeight > 0.f)
	{
		ContentWidget = SNew(SVerticalBox)
			// 顶部间距
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSpacer)
				.Size(FVector2D(0.f, EdgeSpacerHeight))
			]
			// 主要内容
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				ContentWidget
			]
			// 底部间距
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSpacer)
				.Size(FVector2D(0.f, EdgeSpacerHeight))
			];
	}

	// 如果设置了最小高度，用SBox包装
	if (MinWidgetHeight > 0.f)
	{
		return SNew(SBox)
			.MinDesiredHeight(MinWidgetHeight)
			[
				ContentWidget
			];
	}

	return ContentWidget;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// Title风格的Button控件
inline TSharedRef<SWidget> MCWH_TitleButton(
	const FText& Text,
	const FOnClicked& OnClickedDelegate
)
{
	// 获取ToggleButtonCheckbox样式，并映射到Button样式
	const FCheckBoxStyle* ToggleCheckBoxStyle = &FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckbox");
	static const FButtonStyle TitleButtonStyle = FButtonStyle()
		.SetNormal(ToggleCheckBoxStyle->UncheckedImage)
		.SetHovered(ToggleCheckBoxStyle->UncheckedHoveredImage)
		.SetPressed(ToggleCheckBoxStyle->CheckedImage)
		.SetNormalForeground(FSlateColor::UseSubduedForeground())
		.SetHoveredForeground(ToggleCheckBoxStyle->HoveredForeground)
		.SetPressedForeground(ToggleCheckBoxStyle->CheckedForeground)
		.SetDisabledForeground(FSlateColor::UseForeground());

	return SNew(SButton)
		.ButtonStyle(&TitleButtonStyle)
		.ContentPadding(FMargin(6.0f, 2.0f))
		.OnClicked(OnClickedDelegate)
		[
			SNew(STextBlock)
			.Text(Text)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FSlateColor::UseForeground())
			.Justification(ETextJustify::Center)
		];
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// Title风格的CheckBox控件
inline TSharedRef<SWidget> MCWH_TitleCheckBox(
	const FText& Text,
	const FOnCheckStateChanged& OnCheckStateChangedDelegate,
	const TAttribute<ECheckBoxState>& IsCheckedAttribute
)
{
	return SNew(SCheckBox)
		.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
		.IsChecked(IsCheckedAttribute)
		.OnCheckStateChanged(OnCheckStateChangedDelegate)
		[
			SNew(STextBlock)
			.Text(Text)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity_Lambda([IsCheckedAttribute]() -> FSlateColor
			{
				ECheckBoxState CheckState = IsCheckedAttribute.Get();
				return (CheckState == ECheckBoxState::Checked) ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground();
			})
		];
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#endif
