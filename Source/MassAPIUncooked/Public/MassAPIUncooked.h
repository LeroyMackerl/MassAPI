/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#pragma once

#include "Modules/ModuleManager.h"


/**
 * The main NeighborGrid editor module.
 */
class MASSAPIUNCOOKED_API FMassAPIUncookedModule
  : public IModuleInterface
{
  public:

#pragma region IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
#pragma endregion IModuleInterface
};
