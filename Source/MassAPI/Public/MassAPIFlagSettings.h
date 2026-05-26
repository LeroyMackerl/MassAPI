/*
* MassAPI
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MassAPIEnums.h"
#include "MassAPIFlagSettings.generated.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/**
 * Map human-readable flag names (FName) to internal bit positions (EEntityFlags).
 * Designers edit this in Project Settings → Plugins → MassAPI Flags.
 * FlagRegistry starts empty — users or integrating plugins populate it.
 * | 将可读旗标名映射到位位置。注册表初始为空 — 由用户或集成插件填充。
 */
UCLASS(Config=MassAPI, DefaultConfig, meta=(DisplayName="MassAPI Flags"))
class MASSAPI_API UMassAPIFlagSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	/** FName → bit-position mapping. User edits in Project Settings. | FName → 位位置映射 */
	UPROPERTY(Config, EditAnywhere, Category="Flag Registry", meta=(DisplayName="Flag Registry"))
	TMap<FName, EEntityFlags> FlagRegistry;

	//———————— UDeveloperSettings overrides | 设置分类名

	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
	virtual FName GetSectionName() const override { return FName(TEXT("MassAPI Flags")); }

#if WITH_EDITOR
	virtual FText GetSectionText() const override
	{
		return NSLOCTEXT("MassAPI", "FlagSettingsSection", "MassAPI Flags");
	}
	virtual FText GetSectionDescription() const override
	{
		return NSLOCTEXT("MassAPI", "FlagSettingsDesc",
			"Map human-readable flag names (FName) to bit positions (EEntityFlags). Blueprints use the FName; the runtime resolves to the internal bitmask. | 将可读旗标名映射到位位置。");
	}
#endif

	/** Resolve a flag FName to its EEntityFlags bit position. Returns EEntityFlags_MAX if not found. | 将旗标 FName 解析为位位置 */
	static EEntityFlags ResolveFlag(FName FlagName);
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
