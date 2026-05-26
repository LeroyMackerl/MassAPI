/*
* MassAPI
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#include "MassAPIFlagSettings.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

TMap<FName, EEntityFlags> UMassAPIFlagSettings::GetBuiltInDefaults()
{
	TMap<FName, EEntityFlags> Defaults;
	// Low flags (0-63) — stored in Flags int64 | 低位旗标
	Defaults.Add(FName("Appearing"),            EEntityFlags::Flag0);
	Defaults.Add(FName("AppearAnim"),           EEntityFlags::Flag1);
	Defaults.Add(FName("AppearDissolve"),       EEntityFlags::Flag2);
	Defaults.Add(FName("Sleeping"),             EEntityFlags::Flag3);
	Defaults.Add(FName("Patrolling"),           EEntityFlags::Flag4);
	Defaults.Add(FName("Attacking"),            EEntityFlags::Flag5);
	Defaults.Add(FName("AttackAnim"),           EEntityFlags::Flag6);
	Defaults.Add(FName("Falling"),              EEntityFlags::Flag7);
	Defaults.Add(FName("FallAnim"),             EEntityFlags::Flag8);
	Defaults.Add(FName("BeingPushed"),          EEntityFlags::Flag9);
	Defaults.Add(FName("Launching"),            EEntityFlags::Flag10);
	Defaults.Add(FName("Chasing"),              EEntityFlags::Flag11);
	Defaults.Add(FName("Approaching"),          EEntityFlags::Flag12);
	Defaults.Add(FName("BeingHit"),             EEntityFlags::Flag13);
	Defaults.Add(FName("HitAnim"),              EEntityFlags::Flag14);
	Defaults.Add(FName("HitGlow"),              EEntityFlags::Flag15);
	Defaults.Add(FName("HitJiggle"),            EEntityFlags::Flag16);
	Defaults.Add(FName("Dying"),                EEntityFlags::Flag17);
	Defaults.Add(FName("DeathAnim"),            EEntityFlags::Flag18);
	Defaults.Add(FName("DeathDissolve"),        EEntityFlags::Flag19);
	Defaults.Add(FName("Activated"),            EEntityFlags::Flag20);
	Defaults.Add(FName("AI"),                   EEntityFlags::Flag21);
	Defaults.Add(FName("BurstFx"),              EEntityFlags::Flag22);
	Defaults.Add(FName("AttachedFx"),           EEntityFlags::Flag23);
	Defaults.Add(FName("Rendering"),            EEntityFlags::Flag24);
	Defaults.Add(FName("NotRendering"),         EEntityFlags::Flag25);
	Defaults.Add(FName("Agent"),                EEntityFlags::Flag26);
	Defaults.Add(FName("Obstacle"),             EEntityFlags::Flag27);
	Defaults.Add(FName("Projectile"),           EEntityFlags::Flag28);
	Defaults.Add(FName("Prop"),                 EEntityFlags::Flag29);
	Defaults.Add(FName("Slowing"),              EEntityFlags::Flag30);
	Defaults.Add(FName("TemporalDamaging"),     EEntityFlags::Flag31);
	Defaults.Add(FName("RenderingWithParticle"),EEntityFlags::Flag32);
	Defaults.Add(FName("RenderingWithActor"),   EEntityFlags::Flag33);
	Defaults.Add(FName("PauseAnimChange"),      EEntityFlags::Flag34);
	Defaults.Add(FName("PendingDestroy"),       EEntityFlags::Flag35);
	Defaults.Add(FName("Loot"),                 EEntityFlags::Flag36);
	Defaults.Add(FName("AStarMovingTo"),        EEntityFlags::Flag37);
	Defaults.Add(FName("Selected"),             EEntityFlags::Flag38);
	Defaults.Add(FName("BeingSelect"),          EEntityFlags::Flag39);
	Defaults.Add(FName("Idle"),                 EEntityFlags::Flag40);
	Defaults.Add(FName("Returning"),            EEntityFlags::Flag41);
	Defaults.Add(FName("BPTask_MoveTo"),        EEntityFlags::Flag42);
	Defaults.Add(FName("BPTask_ChaseAttack"),   EEntityFlags::Flag43);
	Defaults.Add(FName("Reinforcing"),          EEntityFlags::Flag44);
	Defaults.Add(FName("PlayerDriven"),         EEntityFlags::Flag45);
	return Defaults;
}

void UMassAPIFlagSettings::PostInitProperties()
{
	Super::PostInitProperties();

	// Only seed the CDO once. After user opens Project Settings, the full list is saved to
	// the project .ini and reloaded on next boot, so FlagRegistry is never empty. |
	// 仅在 CDO 首次初始化时从 C++ 注入默认值；用户打开 Project Settings 后完整列表
	// 会被保存到项目 .ini，下次启动时从 ini 加载，FlagRegistry 非空。
	if (!HasAnyFlags(RF_ClassDefaultObject)) return;

	if (FlagRegistry.IsEmpty())
	{
		FlagRegistry = GetBuiltInDefaults();
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
