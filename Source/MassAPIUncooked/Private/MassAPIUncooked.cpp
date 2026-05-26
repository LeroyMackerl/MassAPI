/*
* MassAPI
* Created: 2025
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#include "MassAPIUncooked.h"
#include "Slate/MassAPIPinFactory.h"
#include "Slate/MassAPIFlagPropertyCustomization.h"
#include "EdGraphUtilities.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"    // Required for IMPLEMENT_MODULE
#include "Templates/SharedPointer.h"  // Required for MakeShareable

#define LOCTEXT_NAMESPACE "FMassAPIUncookedModule"

void FMassAPIUncookedModule::StartupModule()
{
	// Register Pin Factory | 注册引脚工厂
	MassAPIPinFactory = MakeShareable(new FMassAPIPinFactory());
	FEdGraphUtilities::RegisterVisualPinFactory(MassAPIPinFactory);

	// Register property customization for query struct flag arrays | 注册查询结构体旗标数组属性定制
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout("FEntityQuery", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMassAPIFlagPropertyCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout("FMassBattleQuery", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMassAPIFlagPropertyCustomization::MakeInstance));
}

void FMassAPIUncookedModule::ShutdownModule()
{
	// Unregister Pin Factory | 注销引脚工厂
	if (MassAPIPinFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualPinFactory(MassAPIPinFactory);
		MassAPIPinFactory.Reset();
	}

	// Unregister property customizations | 注销属性定制
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("FEntityQuery");
		PropertyModule.UnregisterCustomPropertyTypeLayout("FMassBattleQuery");
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMassAPIUncookedModule, MassAPIUncooked)
