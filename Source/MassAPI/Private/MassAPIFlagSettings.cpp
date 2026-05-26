/*
* MassAPI
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#include "MassAPIFlagSettings.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

EEntityFlags UMassAPIFlagSettings::ResolveFlag(FName FlagName)
{
	const UMassAPIFlagSettings* Settings = GetDefault<UMassAPIFlagSettings>();
	if (!Settings)
	{
		return EEntityFlags::EEntityFlags_MAX;
	}
	const EEntityFlags* Found = Settings->FlagRegistry.Find(FlagName);
	return Found ? *Found : EEntityFlags::EEntityFlags_MAX;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
