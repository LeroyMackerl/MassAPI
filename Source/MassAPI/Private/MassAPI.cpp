/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#include "MassAPI.h"

#define LOCTEXT_NAMESPACE "FMassAPIModule"

void FMassAPIModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FMassAPIModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMassAPIModule, MassAPI)