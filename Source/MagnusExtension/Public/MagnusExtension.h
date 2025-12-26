#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class MAGNUSEXTENSION_API FMagnusExtensionModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

DECLARE_LOG_CATEGORY_EXTERN(LogMagnusExtension, Log, All);
