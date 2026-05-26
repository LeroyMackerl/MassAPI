/*
* MassAPI
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#include "Slate/MassAPIFlagPropertyCustomization.h"

#if WITH_EDITOR
#include "MassAPIFlagSettings.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

TSharedRef<IPropertyTypeCustomization> FMassAPIFlagPropertyCustomization::MakeInstance()
{
	return MakeShareable(new FMassAPIFlagPropertyCustomization);
}

void FMassAPIFlagPropertyCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	];
	HeaderRow.ValueContent()
	[
		PropertyHandle->CreatePropertyValueWidget()
	];
}

void FMassAPIFlagPropertyCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	uint32 NumChildren = 0;
	PropertyHandle->GetNumChildren(NumChildren);

	for (uint32 i = 0; i < NumChildren; ++i)
	{
		TSharedRef<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(i).ToSharedRef();

		if (IsFlagArrayProperty(ChildHandle))
		{
			// Use FDetailArrayBuilder with per-element ComboBox | 使用 FDetailArrayBuilder，每个元素渲染 ComboBox
			TSharedRef<FDetailArrayBuilder> ArrayBuilder = MakeShareable(new FDetailArrayBuilder(ChildHandle, false, true, true));
			ArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FMassAPIFlagPropertyCustomization::OnGenerateFlagArrayElement));
			ChildBuilder.AddCustomBuilder(ArrayBuilder);
		}
		else
		{
			ChildBuilder.AddProperty(ChildHandle);
		}
	}
}

bool FMassAPIFlagPropertyCustomization::IsFlagArrayProperty(const TSharedRef<IPropertyHandle>& ChildHandle) const
{
	FString PropName = ChildHandle->GetProperty()->GetName();
	return PropName.Contains(TEXT("FlagsList")) || PropName.Contains(TEXT("FlagNamesList"));
}

void FMassAPIFlagPropertyCustomization::OnGenerateFlagArrayElement(TSharedRef<IPropertyHandle> ElementHandle, int32 Index, IDetailChildrenBuilder& ChildrenBuilder)
{
	// Build flag name options from settings | 从设置构建旗标名选项
	TSharedRef< TArray<FName> > Options = MakeShareable(new TArray<FName>());
	if (const UMassAPIFlagSettings* Settings = GetDefault<UMassAPIFlagSettings>())
	{
		Settings->FlagRegistry.GetKeys(*Options);
		Options->Sort([](const FName& A, const FName& B) { return A.FastLess(B); });
	}

	// Get current value | 获取当前值
	FName CurrentValue;
	ElementHandle->GetValue(CurrentValue);

	// Find initially selected item | 查找初始选中项
	TSharedPtr<FName> InitiallySelected;
	for (const FName& Opt : *Options)
	{
		if (Opt == CurrentValue)
		{
			InitiallySelected = MakeShareable(new FName(Opt));
			break;
		}
	}

	// ComboBox that writes back to the property handle | 下拉框修改属性值
	ChildrenBuilder.AddCustomRow(ElementHandle->GetPropertyDisplayName())
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::AsNumber(Index))
			.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
		]
		.ValueContent()
		[
			SNew(SComboBox<FName>)
			.OptionsSource(&Options.Get())
			.InitiallySelectedItem(InitiallySelected.IsValid() ? *InitiallySelected : NAME_None)
			.OnSelectionChanged_Lambda([ElementHandle](FName Item, ESelectInfo::Type)
			{
				if (!Item.IsNone())
				{
					ElementHandle->SetValue(Item);
				}
			})
			.OnGenerateWidget_Lambda([](FName Item)
			{
				return SNew(SBox)
					.Padding(FMargin(6.0f, 3.0f))
					[
						SNew(STextBlock)
						.Text(FText::FromName(Item))
						.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
					];
			})
			[
				SNew(STextBlock)
				.Text_Lambda([ElementHandle]() -> FText
				{
					FName Val;
					ElementHandle->GetValue(Val);
					return Val.IsNone() ? FText::FromString(TEXT("Select Flag...")) : FText::FromName(Val);
				})
				.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
		];
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
#endif // WITH_EDITOR
