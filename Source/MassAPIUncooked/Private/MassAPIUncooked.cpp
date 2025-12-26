// Leroy Works & Ember, All Rights Reserved.

#include "MassAPIUncooked.h"
#include "Slate/MassAPIPinFactory.h"
#include "EdGraphUtilities.h"
#include "Modules/ModuleManager.h"    // Required for IMPLEMENT_MODULE
#include "Templates/SharedPointer.h"  // Required for MakeShareable

#define LOCTEXT_NAMESPACE "FMassAPIUncookedModule"

void FMassAPIUncookedModule::StartupModule()
{
	// 创建 Pin Factory 实例
	MassAPIPinFactory = MakeShareable(new FMassAPIPinFactory());

	// 注册 Pin Factory 到编辑器
	FEdGraphUtilities::RegisterVisualPinFactory(MassAPIPinFactory);
}

void FMassAPIUncookedModule::ShutdownModule()
{
	// 注销 Pin Factory
	if (MassAPIPinFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualPinFactory(MassAPIPinFactory);
		MassAPIPinFactory.Reset();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMassAPIUncookedModule, MassAPIUncooked)
