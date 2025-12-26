// Leroy Works & Ember, All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FMassAPIPinFactory;

class MASSAPIUNCOOKED_API FMassAPIUncookedModule : public IModuleInterface
{

public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	// Pin Factory，用于创建自定义的蓝图引脚 Widget
	TSharedPtr<FMassAPIPinFactory> MassAPIPinFactory;

};
