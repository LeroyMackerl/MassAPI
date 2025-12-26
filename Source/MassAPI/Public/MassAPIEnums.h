/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "MassAPIEnums.generated.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// DataSource类型枚举
enum class EMassFragmentSourceDataType : uint8
{
	None,				// 未连接或无效
	EntityHandle,		// FEntityHandle - 用于修改实体
	EntityTemplateData	// FEntityTemplateData - 用于修改模板
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/**
 * 定义实体可拥有的动态标志 (最多 64 个)。
 * 这是一个标准的 uint8 UENUM，用于在蓝图中填充数组。
 * C++ 逻辑会将其索引 (0-63) 转换为 int64 位掩码。
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
	/** 必须是最后一个，用于计数 */
	EEntityFlags_MAX UMETA(Hidden)
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
