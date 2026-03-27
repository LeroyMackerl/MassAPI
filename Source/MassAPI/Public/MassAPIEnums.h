/*
* MassAPI
* Created: 2025
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "MassAPIEnums.generated.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// DataSource类型枚举
enum class EMassFragmentSourceDataType : uint8
{
	None,						// 未连接或无效
	EntityHandle,				// FEntityHandle - 用于修改实体
	EntityTemplateData,			// FEntityTemplateData - 用于修改模板
	EntityHandleArray,			// TArray<FEntityHandle> - 用于修改多个实体
	EntityTemplateDataArray		// TArray<FEntityTemplateData> - 用于修改多个模板
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/**
 * 定义实体可拥有的动态标志 (最多 128 个)。
 * 这是一个标准的 uint8 UENUM，用于在蓝图中填充数组。
 * C++ 逻辑会将其索引 (0-63) 转换为 Flags (低位 int64)，
 * 索引 (64-127) 转换为 FlagsHigh (高位 int64)。
 */
UENUM(BlueprintType)
enum class EEntityFlags : uint8
{
	Flag0,
	Flag1,
	Flag2,
	Flag3,
	Flag4,
	Flag5,
	Flag6,
	Flag7,
	Flag8,
	Flag9,
	Flag10,
	Flag11,
	Flag12,
	Flag13,
	Flag14,
	Flag15,
	Flag16,
	Flag17,
	Flag18,
	Flag19,
	Flag20,
	Flag21,
	Flag22,
	Flag23,
	Flag24,
	Flag25,
	Flag26,
	Flag27,
	Flag28,
	Flag29,
	Flag30,
	Flag31,
	Flag32,
	Flag33,
	Flag34,
	Flag35,
	Flag36,
	Flag37,
	Flag38,
	Flag39,
	Flag40,
	Flag41,
	Flag42,
	Flag43,
	Flag44,
	Flag45,
	Flag46,
	Flag47,
	Flag48,
	Flag49,
	Flag50,
	Flag51,
	Flag52,
	Flag53,
	Flag54,
	Flag55,
	Flag56,
	Flag57,
	Flag58,
	Flag59,
	Flag60,
	Flag61,
	Flag62,
	Flag63,
	// High flags (64-127) - stored in FlagsHigh int64
	Flag64,
	Flag65,
	Flag66,
	Flag67,
	Flag68,
	Flag69,
	Flag70,
	Flag71,
	Flag72,
	Flag73,
	Flag74,
	Flag75,
	Flag76,
	Flag77,
	Flag78,
	Flag79,
	Flag80,
	Flag81,
	Flag82,
	Flag83,
	Flag84,
	Flag85,
	Flag86,
	Flag87,
	Flag88,
	Flag89,
	Flag90,
	Flag91,
	Flag92,
	Flag93,
	Flag94,
	Flag95,
	Flag96,
	Flag97,
	Flag98,
	Flag99,
	Flag100,
	Flag101,
	Flag102,
	Flag103,
	Flag104,
	Flag105,
	Flag106,
	Flag107,
	Flag108,
	Flag109,
	Flag110,
	Flag111,
	Flag112,
	Flag113,
	Flag114,
	Flag115,
	Flag116,
	Flag117,
	Flag118,
	Flag119,
	Flag120,
	Flag121,
	Flag122,
	Flag123,
	Flag124,
	Flag125,
	Flag126,
	Flag127,
	/** 必须是最后一个，用于计数 */
	EEntityFlags_MAX UMETA(Hidden)
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
