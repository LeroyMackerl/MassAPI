#include "MagnusUtilities.h"

#if WITH_EDITOR
#include "CustomProperty/Magnus_CustomPropertyHelper.h"
#endif

#define LOCTEXT_NAMESPACE "FMagnusUtilitiesModule"

void FMagnusUtilitiesModule::StartupModule()
{
	#if WITH_EDITOR
	FMCPHCustomPropertyAdder::OnRegisterCustomProperties().Broadcast();
	#endif
}

void FMagnusUtilitiesModule::ShutdownModule()
{
	#if WITH_EDITOR
    FMCPHCustomPropertyAdder::OnUnregisterCustomProperties().Broadcast();
	#endif
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FMagnusUtilitiesModule, MagnusUtilities)