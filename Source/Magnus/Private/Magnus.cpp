/*
* Magnus
* Author: Ember, All Rights Reserved.
*/

#include "Magnus.h"

#if WITH_EDITOR
#include "CustomProperty/Magnus_CustomPropertyHelper.h"
#endif

DEFINE_LOG_CATEGORY(LogMagnus);

#define LOCTEXT_NAMESPACE "FMagnusModule"

void FMagnusModule::StartupModule()
{
#if WITH_EDITOR
	FMCPHCustomPropertyAdder::OnRegisterCustomProperties().Broadcast();
#endif
}

void FMagnusModule::ShutdownModule()
{
#if WITH_EDITOR
	FMCPHCustomPropertyAdder::OnUnregisterCustomProperties().Broadcast();
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMagnusModule, Magnus)
