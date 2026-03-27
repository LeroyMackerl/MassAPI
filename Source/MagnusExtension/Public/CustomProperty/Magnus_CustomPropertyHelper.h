/*
* Magnus Extension
* Author: Ember, All Rights Reserved.
*/

#pragma once


#if WITH_EDITOR

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "PropertyCustomizationHelpers.h"
#include "IPropertyUtilities.h"
#include "IPropertyTypeCustomization.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "PropertyHandle.h"

#include "CustomProperty/Magnus_CustomPropertyWidgetCreator.h"

using namespace PropertyCustomizationHelpers;

#endif

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if WITH_EDITOR

//———————— Define						    																        ————

// 开始定义自定义属性类（自动包含 MakeInstance）
#define BEGIN_MCPH_DEFINE(StructName) \
class FMagnusCustomProperty_##StructName : public IPropertyTypeCustomization \
{ \
private: \
	typedef FMagnusCustomProperty_##StructName ThisClass; \
	typedef StructName TargetStructType; \
\
public: \
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() \
	{ \
		return MakeShareable(new ThisClass); \
	}

// 结束自定义属性类定义（与 BEGIN_MCPH_DEFINE 配对）
#define END_MCPH_DEFINE \
};

// 获取自定义属性类名 (双层宏用于确保参数在 ## 之前被展开)
#define MCPH_CLASS_IMPL(StructName) FMagnusCustomProperty_##StructName
#define MCPH_CLASS(StructName) MCPH_CLASS_IMPL(StructName)

//———————— Header						    																        ————

// Header 声明
#define MCPH_HEADER_CUSTOM \
virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

// Header 空实现
#define MCPH_HEADER_EMPTY \
virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}

// Header 默认实现（只显示名称）
#define MCPH_HEADER_DEFAULT \
virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override \
{ \
	HeaderRow.NameContent() \
	[ \
		PropertyHandle->CreatePropertyNameWidget() \
	] \
	.ValueContent() \
	[ \
		SNullWidget::NullWidget \
	]; \
}

//———————— Children						    																        ————

// Children 声明
#define MCPH_CHILDREN_CUSTOM \
virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

// Children 空实现
#define MCPH_CHILDREN_EMPTY \
virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}

// Children 默认实现（显示所有子属性）
#define MCPH_CHILDREN_DEFAULT \
virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override \
{ \
	uint32 NumChildren; \
	PropertyHandle->GetNumChildren(NumChildren); \
	for (uint32 i = 0; i < NumChildren; i++) \
	{ \
		TSharedRef<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(i).ToSharedRef(); \
		ChildBuilder.AddProperty(ChildHandle); \
	} \
}

//———————— Registry						    																        ————

// 注册自定义属性模板函数
template<typename StructType, typename CustomizationType>
void MCPH_RegisterCustomProperty()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		StructType::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&CustomizationType::MakeInstance)
	);
}

// 注销自定义属性模板函数
template<typename StructType>
void MCPH_UnregisterCustomProperty()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(StructType::StaticStruct()->GetFName());
	}
}

// 手动注册宏
#define MCPH_REGISTER(StructType) \
MCPH_RegisterCustomProperty<StructType, FMagnusCustomProperty_##StructType>();

// 手动注销宏
#define MCPH_UNREGISTER(StructType) \
MCPH_UnregisterCustomProperty<StructType>();

//———————— Auto-Registration				    															        ————

// 委托声明：注册和注销自定义属性
DECLARE_MULTICAST_DELEGATE(FOnMCPHRegisterCustomProperties);
DECLARE_MULTICAST_DELEGATE(FOnMCPHUnregisterCustomProperties);

// 自定义属性自动注册器基类
class FMCPHCustomPropertyAdder
{
public:
	FMCPHCustomPropertyAdder();
	virtual ~FMCPHCustomPropertyAdder() = default;

	// 派生类实现：注册/注销具体的自定义属性
	virtual void RegisterProperty() = 0;
	virtual void UnregisterProperty() = 0;

	// 获取全局委托
	static FOnMCPHRegisterCustomProperties& OnRegisterCustomProperties();
	static FOnMCPHUnregisterCustomProperties& OnUnregisterCustomProperties();
};

// 内联实现
inline FMCPHCustomPropertyAdder::FMCPHCustomPropertyAdder()
{
	// 自动绑定到注册委托
	OnRegisterCustomProperties().AddRaw(this, &FMCPHCustomPropertyAdder::RegisterProperty);
	OnUnregisterCustomProperties().AddRaw(this, &FMCPHCustomPropertyAdder::UnregisterProperty);
}

inline FOnMCPHRegisterCustomProperties& FMCPHCustomPropertyAdder::OnRegisterCustomProperties()
{
	// 静态局部变量，保证在任何静态初始化之前构造
	static FOnMCPHRegisterCustomProperties RegisterDelegate;
	return RegisterDelegate;
}

inline FOnMCPHUnregisterCustomProperties& FMCPHCustomPropertyAdder::OnUnregisterCustomProperties()
{
	// 静态局部变量，保证在任何静态初始化之前构造
	static FOnMCPHUnregisterCustomProperties UnregisterDelegate;
	return UnregisterDelegate;
}

// 自动生成注册器类和全局静态实例
#define MCPH_AUTO_REGISTER(StructType) \
class FMCPHCustomPropertyAdder_##StructType : public FMCPHCustomPropertyAdder \
{ \
public: \
	virtual void RegisterProperty() override \
	{ \
		MCPH_RegisterCustomProperty<StructType, FMagnusCustomProperty_##StructType>(); \
	} \
	virtual void UnregisterProperty() override \
	{ \
		MCPH_UnregisterCustomProperty<StructType>(); \
	} \
}; \
static FMCPHCustomPropertyAdder_##StructType GMCPHAutoRegister_##StructType;

//———————— Implementation (CPP)			    																        ————

// 开始实现块（注册自定义属性）
#define BEGIN_MCPH_IMPLEMENT(StructName) \
_Pragma("warning(push)") \
_Pragma("warning(disable: 4996)") \
MCPH_AUTO_REGISTER(StructName)

// Header 实现（需要传入 StructName）
#define MCPH_HEADER_IMPLEMENT \
void MCPH_THIS_CLASS::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)

// Children 实现（需要传入 StructName）
#define MCPH_CHILDREN_IMPLEMENT \
void MCPH_THIS_CLASS::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)

// 获取当前自定义属性类（需在每个 .cpp 文件顶部定义 MCPH_CURRENT_STRUCT）
// 用法: 在 BEGIN_MCPH_IMPLEMENT 之前添加 #define MCPH_CURRENT_STRUCT YourStructName
#define MCPH_THIS_CLASS MCPH_CLASS(MCPH_CURRENT_STRUCT)

// 结束实现块
#define END_MCPH_IMPLEMENT \
_Pragma("warning(pop)") \
/* 在文件末尾 undef MCPH_CURRENT_STRUCT 以避免污染 */

#endif // WITH_EDITOR

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
