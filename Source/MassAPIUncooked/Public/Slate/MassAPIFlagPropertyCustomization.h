/*
* MassAPI
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "IPropertyTypeCustomization.h"
#include "DetailWidgetRow.h"

class IPropertyHandle;
class IDetailChildrenBuilder;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/**
 * Property customization for FEntityQuery and FMassBattleQuery structs.
 * Replaces TArray<FName> flag arrays with ComboBox-per-element via FDetailArrayBuilder.
 * | 属性定制：用 FDetailArrayBuilder 为 FEntityQuery/FMassBattleQuery 的 TArray<FName> 旗标数组
 *   每个元素渲染 ComboBox 下拉。
 */
class MASSAPIUNCOOKED_API FMassAPIFlagPropertyCustomization : public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//———————— IPropertyTypeCustomization

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:

	/** FDetailArrayBuilder per-element callback: renders a ComboBox for one FName element. | 每个数组元素的 ComboBox 渲染 */
	void OnGenerateFlagArrayElement(TSharedRef<IPropertyHandle> ElementHandle, int32 Index, IDetailChildrenBuilder& ChildrenBuilder);

	/** Check whether a child property is a flag array and should be customized. | 检查子属性是否是需要定制的旗标数组 */
	bool IsFlagArrayProperty(const TSharedRef<IPropertyHandle>& ChildHandle) const;
};

#endif // WITH_EDITOR

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
