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
 * Built-in defaults come from C++ (GetBuiltInDefaults); ini stores user extensions/overrides.
 * | 将可读旗标名映射到位位置。内置默认值来自 C++；ini 存储用户扩展/覆盖。
 */
UCLASS(Config=MassAPI, DefaultConfig, meta=(DisplayName="MassAPI Flags"))
class MASSAPI_API UMassAPIFlagSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	/** FName → bit-position mapping. User edits in Project Settings. Built-in defaults auto-populated at first boot. | FName → 位位置映射 */
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

	virtual void PostInitProperties() override;

	//———————— Built-in defaults | 内置默认映射

	/** Return the 46 built-in flag name → bit-position mappings matching EBattleFlags. | 返回与 EBattleFlags 匹配的 46 个内置映射 */
	static TMap<FName, EEntityFlags> GetBuiltInDefaults();
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
