/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "MassEntityTypes.h"
#include "MassEntityManager.h"
#include "MassEntityTemplate.h"
#include "MassAPIEnums.h" // 包含新的枚举头文件
#include "MassAPIStructs.generated.h" 


#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
	// > UE 5.7 accessor methods
	#define MASS_TAG_STRUCT BaseStruct = "/Script/MassEntity.MassTag"
	#define MASS_FRAGMENT_STRUCT BaseStruct = "/Script/MassEntity.MassFragment"
	#define MASS_SHARED_FRAGMENT_STRUCT BaseStruct = "/Script/MassEntity.MassSharedFragment"
	#define MASS_ENTITY_STRUCT BaseStruct = "/Script/MassEntity.MassConstSharedFragment"
#else
	// < UE 5.7 accessor methods
	#define MASS_TAG_STRUCT BaseStruct = "MassTag"
	#define MASS_FRAGMENT_STRUCT BaseStruct = "MassFragment"
	#define MASS_SHARED_FRAGMENT_STRUCT BaseStruct = "MassSharedFragment"
	#define MASS_ENTITY_STRUCT BaseStruct = "MassConstSharedFragment"
#endif


/**
 * FEntityHandle is a blueprintable wrapper of MassEntityHandle
 */
USTRUCT(BlueprintType)
struct MASSAPI_API FEntityHandle
{
	GENERATED_BODY()

public:
	// Default constructor
	FEntityHandle() = default;

	// Construct from an FMassEntityHandle
	FORCEINLINE FEntityHandle(const FMassEntityHandle& MassHandle) : Index(MassHandle.Index), Serial(MassHandle.SerialNumber) {}

	// Conversion to FMassEntityHandle (this is the key to making it work seamlessly)
	FORCEINLINE operator FMassEntityHandle() const
	{
		return FMassEntityHandle(Index, Serial);
	}

	// Note: !IsEmptyHandle() does not mean the handle is valid, it could be expired
	FORCEINLINE bool IsEmptyHandle() const
	{
		return Index != 0 && Serial != 0;
	}

	// Blueprint-accessible entity index
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "MassAPI|EntityHandle")
	int32 Index = 0;

	// Blueprint-accessible serial number
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "MassAPI|EntityHandle")
	int32 Serial = 0;

	// Comparison operator (used by EqualEqual_EntityHandle in Blueprints)
	FORCEINLINE bool operator==(const FEntityHandle& Other) const
	{
		return Index == Other.Index && Serial == Other.Serial;
	}

	// Support for Blueprint comparisons (for TMap, TSet, etc.)
	FORCEINLINE friend uint32 GetTypeHash(const FEntityHandle& Handle)
	{
		return HashCombine(GetTypeHash(Handle.Index), GetTypeHash(Handle.Serial));
	}
};

/**
 * (新) 存储不导致 Archetype 迁移的动态标志。
 * 使用 int64 作为位掩码，用于高性能的内部存储和查询。
 * 蓝图将使用 MassAPIBPFnLib 中的辅助函数来操作这个位掩码。
 * (已重命名: FMassEntityFlagFragment -> FEntityFlagFragment)
 */
USTRUCT(BlueprintType)
struct MASSAPI_API FEntityFlagFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Flags")
	int64 Flags = 0;

	/**
	 * (新) 检查此Fragment是否设置了某个特定标志。
	 * @param Flag The flag to check.
	 * @return True if the flag is set, false otherwise.
	 */
	FORCEINLINE bool HasFlag(const EEntityFlags Flag) const
	{
		if (Flag >= EEntityFlags::EEntityFlags_MAX) return false;
		return (Flags & (1LL << static_cast<uint8>(Flag))) != 0;
	}

	/**
	 * (新) 在此Fragment上设置一个特定标志。
	 * @param Flag The flag to set (turn on).
	 */
	FORCEINLINE void SetFlag(const EEntityFlags Flag)
	{
		if (Flag >= EEntityFlags::EEntityFlags_MAX) return;
		Flags |= (1LL << static_cast<uint8>(Flag));
	}

	/**
	 * (新) 在此Fragment上清除一个特定标志。
	 * @param Flag The flag to clear (turn off).
	 */
	FORCEINLINE void ClearFlag(const EEntityFlags Flag)
	{
		if (Flag >= EEntityFlags::EEntityFlags_MAX) return;
		Flags &= ~(1LL << static_cast<uint8>(Flag));
	}

	/**
	 * (新) 根据布尔值在此Fragment上设置或清除一个特定标志。
	 * @param Flag The flag to change.
	 * @param bValue True to set the flag, false to clear it.
	 */
	FORCEINLINE void SetFlagValue(const EEntityFlags Flag, const bool bValue)
	{
		if (bValue)
		{
			SetFlag(Flag);
		}
		else
		{
			ClearFlag(Flag);
		}
	}
};


/**
 * FEntityQuery allows for individual entity fragment and tag composition matching
 */
USTRUCT(BlueprintType)
struct MASSAPI_API FEntityQuery
{
	GENERATED_BODY()

protected:
	/** (新) 用于在编辑器中隐藏此属性的 EditCondition */
	UPROPERTY()
	bool bShowBaseFlagsInEditor = true;

public:
	// ----------- 构造函数 -----------

	/** 默认构造函数 */
	FEntityQuery()
	{
		MarkCacheDirty();
	}

	/** 构造函数（带缓存控制） */
	FEntityQuery(bool bUpdateCache)
	{
		if (bUpdateCache)
		{
			MarkCacheDirty();
		}
		else
		{
			bIsAllCompDirty = false;
			bIsAnyCompDirty = false;
			bIsNoneCompDirty = false;
			bIsFlagsCacheDirty = false;
		}
	}

	/** 方便的 C++ 构造函数 */
	FEntityQuery
	(
		TArray<TObjectPtr<UScriptStruct>> InAll,
		TArray<TObjectPtr<UScriptStruct>> InAny,
		TArray<TObjectPtr<UScriptStruct>> InNone
	)
		: AllList(MoveTemp(InAll))
		, AnyList(MoveTemp(InAny))
		, NoneList(MoveTemp(InNone))
	{
		MarkCacheDirty();
		UpdateCache(); // C++ 构造时立即更新
	}

	FEntityQuery
	(
		TArray<TObjectPtr<UScriptStruct>> InAll,
		TArray<TObjectPtr<UScriptStruct>> InAny,
		TArray<TObjectPtr<UScriptStruct>> InNone,
		bool bUpdateCache
	)
		: AllList(MoveTemp(InAll))
		, AnyList(MoveTemp(InAny))
		, NoneList(MoveTemp(InNone))
	{
		if (bUpdateCache)
		{
			MarkCacheDirty();
		}
		else
		{
			bIsAllCompDirty = false;
			bIsAnyCompDirty = false;
			bIsNoneCompDirty = false;
			bIsFlagsCacheDirty = false;
		}

		if (bUpdateCache)
		{
			UpdateCache();
		}
	}

	// ----------- Fluent API / Builder -----------

	/**
	 * Add an "AND" query condition. The entity must have ALL the specified Fragments/Tags.
	 * @tparam TArgs Subclasses of Mass component types.
	 * @return Returns a reference to self to support chained calls.
	 */
	template<typename... TArgs>
	FORCEINLINE FEntityQuery& All()&
	{
		(CheckType<TArgs>(), ...); // Compile-time type check
		(AllList.Add(TArgs::StaticStruct()), ...);
		bIsAllCompDirty = true;
		return *this;
	}
	template<typename... TArgs>
	FORCEINLINE FEntityQuery&& All()&&
	{
		bIsAllCompDirty = true;
		return MoveTemp(this->All<TArgs...>());
	}

	/**
	 * Add an "OR" query condition. The entity must have AT LEAST ONE of the specified Fragments/Tags.
	 * @tparam TArgs Subclasses of Mass component types.
	 * @return Returns a reference to self to support chained calls.
	 */
	template<typename... TArgs>
	FORCEINLINE FEntityQuery& Any()&
	{
		(CheckType<TArgs>(), ...);
		(AnyList.Add(TArgs::StaticStruct()), ...);
		bIsAnyCompDirty = true;
		return *this;
	}
	template<typename... TArgs>
	FORCEINLINE FEntityQuery&& Any()&&
	{
		bIsAnyCompDirty = true;
		return MoveTemp(this->Any<TArgs...>());
	}

	/**
	 * Add a "NOT" query condition. The entity must have NONE of the specified Fragments/Tags.
	 * @tparam TArgs Subclasses of Mass component types.
	 * @return Returns a reference to self to support chained calls.
	 */
	template<typename... TArgs>
	FORCEINLINE FEntityQuery& None()&
	{
		(CheckType<TArgs>(), ...);
		(NoneList.Add(TArgs::StaticStruct()), ...);
		bIsNoneCompDirty = true;
		return *this;
	}
	template<typename... TArgs>
	FORCEINLINE FEntityQuery&& None()&&
	{
		bIsNoneCompDirty = true;
		return MoveTemp(this->None<TArgs...>());
	}

	// ----------- 核心数据 UPROPERTY -----------

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Query", meta = (Tooltip = "Entity traits: AND, the entity's Mass components must match this list completely.", DisplayName = "AllComposition"))
	TArray<TObjectPtr<UScriptStruct>> AllList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Query", meta = (Tooltip = "Entity traits: OR, the entity's Mass components must have at least one from this list.", DisplayName = "AnyComposition"))
	TArray<TObjectPtr<UScriptStruct>> AnyList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Query", meta = (Tooltip = "Entity traits: NOT, the entity's Mass components must not match this list at all.", DisplayName = "NoneComposition"))
	TArray<TObjectPtr<UScriptStruct>> NoneList;

	// ----------- (新) 动态标志查询 (面向用户) -----------

	/** (新) 标志位查询：AND，实体的标志位必须包含所有这些标志。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Flags", meta = (Tooltip = "AND: 实体必须拥有所有这些标志。", DisplayName = "AllFlags", EditCondition = "bShowBaseFlagsInEditor"))
	TArray<EEntityFlags> AllFlagsList;

	/** (新) 标志位查询：OR，实体的标志位必须包含至少一个标志。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Flags", meta = (Tooltip = "OR: 实体必须拥有至少一个标志。", DisplayName = "AnyFlags", EditCondition = "bShowBaseFlagsInEditor"))
	TArray<EEntityFlags> AnyFlagsList;

	/** (新) 标志位查询：NOT，实体的标志位必须不包含任何这些标志。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Flags", meta = (Tooltip = "NOT: 实体必须没有任何这些标志。", DisplayName = "NoneFlags", EditCondition = "bShowBaseFlagsInEditor"))
	TArray<EEntityFlags> NoneFlagsList;


	// ----------- 缓存 Descriptors -----------

private:
	mutable FMassArchetypeCompositionDescriptor AllComposition;
	mutable FMassArchetypeCompositionDescriptor AnyComposition;
	mutable FMassArchetypeCompositionDescriptor NoneComposition;

	// (新) 标志位缓存
	mutable int64 AllFlagsBitmask_Cache = 0;
	mutable int64 AnyFlagsBitmask_Cache = 0;
	mutable int64 NoneFlagsBitmask_Cache = 0;

	mutable bool bIsAllCompDirty = false;
	mutable bool bIsAnyCompDirty = false;
	mutable bool bIsNoneCompDirty = false;
	mutable bool bIsFlagsCacheDirty = false;

public:
	// ----------- 公共接口 -----------

	const FMassArchetypeCompositionDescriptor& GetAllComposition() const
	{
		if (bIsAllCompDirty) { BuildCompositionFromStructs(AllComposition, AllList); bIsAllCompDirty = false; }
		return AllComposition;
	}
	const FMassArchetypeCompositionDescriptor& GetAnyComposition() const
	{
		if (bIsAnyCompDirty) { BuildCompositionFromStructs(AnyComposition, AnyList); bIsAnyCompDirty = false; }
		return AnyComposition;
	}
	const FMassArchetypeCompositionDescriptor& GetNoneComposition() const
	{
		if (bIsNoneCompDirty) { BuildCompositionFromStructs(NoneComposition, NoneList); bIsNoneCompDirty = false; }
		return NoneComposition;
	}

	/** (新) 获取缓存的 All 标志位掩码 */
	int64 GetAllFlagsBitmask() const
	{
		if (bIsFlagsCacheDirty) { BuildFlagsCache(); }
		return AllFlagsBitmask_Cache;
	}
	/** (新) 获取缓存的 Any 标志位掩码 */
	int64 GetAnyFlagsBitmask() const
	{
		if (bIsFlagsCacheDirty) { BuildFlagsCache(); }
		return AnyFlagsBitmask_Cache;
	}
	/** (新) 获取缓存的 None 标志位掩码 */
	int64 GetNoneFlagsBitmask() const
	{
		if (bIsFlagsCacheDirty) { BuildFlagsCache(); }
		return NoneFlagsBitmask_Cache;
	}

	/** (新) 标记所有缓存为脏 */
	FORCEINLINE void MarkCacheDirty()
	{
		bIsAllCompDirty = true;
		bIsAnyCompDirty = true;
		bIsNoneCompDirty = true;
		bIsFlagsCacheDirty = true;
	}

private:
	// ----------- 私有辅助函数 -----------

	/** Enhanced compile-time type check helper - now supports all Mass component types */
	template<typename T>
	static constexpr void CheckType()
	{
		static_assert(
			TIsDerivedFrom<T, FMassFragment>::IsDerived ||
			TIsDerivedFrom<T, FMassTag>::IsDerived ||
			TIsDerivedFrom<T, FMassChunkFragment>::IsDerived ||
			TIsDerivedFrom<T, FMassSharedFragment>::IsDerived ||
			TIsDerivedFrom<T, FMassConstSharedFragment>::IsDerived,
			"Template arguments for All, Any, None must be derived from FMassFragment, FMassTag, FMassChunkFragment, FMassSharedFragment, or FMassConstSharedFragment."
			);
	}

	/** (新) 更新所有缓存 */
	void UpdateCache() const
	{
		if (bIsAllCompDirty)
		{
			BuildCompositionFromStructs(AllComposition, AllList);
			bIsAllCompDirty = false;
		}
		if (bIsAnyCompDirty)
		{
			BuildCompositionFromStructs(AnyComposition, AnyList);
			bIsAnyCompDirty = false;
		}
		if (bIsNoneCompDirty)
		{
			BuildCompositionFromStructs(NoneComposition, NoneList);
			bIsNoneCompDirty = false;
		}
		if (bIsFlagsCacheDirty)
		{
			BuildFlagsCache();
		}
	}

	/** (新) 构建标志位掩码缓存 */
	void BuildFlagsCache() const
	{
		if (!bIsFlagsCacheDirty) return;

		AllFlagsBitmask_Cache = 0;
		AnyFlagsBitmask_Cache = 0;
		NoneFlagsBitmask_Cache = 0;

		// 1LL << X 确保我们执行 64 位位移
		for (const EEntityFlags Flag : AllFlagsList)
		{
			// 检查 EEntityFlags_MAX，尽管 uint8(Flag) 永远不会超过 255
			if (Flag < EEntityFlags::EEntityFlags_MAX)
			{
				AllFlagsBitmask_Cache |= (1LL << static_cast<uint8>(Flag));
			}
		}
		for (const EEntityFlags Flag : AnyFlagsList)
		{
			if (Flag < EEntityFlags::EEntityFlags_MAX)
			{
				AnyFlagsBitmask_Cache |= (1LL << static_cast<uint8>(Flag));
			}
		}
		for (const EEntityFlags Flag : NoneFlagsList)
		{
			if (Flag < EEntityFlags::EEntityFlags_MAX)
			{
				NoneFlagsBitmask_Cache |= (1LL << static_cast<uint8>(Flag));
			}
		}

		bIsFlagsCacheDirty = false;
	}


	/** Enhanced composition builder function - now supports all Mass component types */
	void BuildCompositionFromStructs
	(
		FMassArchetypeCompositionDescriptor& OutComposition,
		const TArray<TObjectPtr<UScriptStruct>>& InStructs
	) const
	{
		FMassFragmentBitSet Fragments;                   // Regular Fragment
		FMassTagBitSet Tags;                             // Tag
		FMassChunkFragmentBitSet ChunkFragments;         // ChunkFragment
		FMassSharedFragmentBitSet SharedFragments;       // SharedFragment
		FMassConstSharedFragmentBitSet ConstSharedFragments; // ConstSharedFragment

		for (const TObjectPtr<UScriptStruct>& Struct : InStructs)
		{
			if (!IsValid(Struct.Get()))
			{
				continue;
			}
			// Check if the struct is a subclass of FMassFragment
			if (Struct->IsChildOf(FMassFragment::StaticStruct()))
			{
				Fragments.Add(*Struct);
			}
			// Check if the struct is a subclass of FMassTag
			else if (Struct->IsChildOf(FMassTag::StaticStruct()))
			{
				Tags.Add(*Struct);
			}
			// Check if the struct is a subclass of FMassChunkFragment
			else if (Struct->IsChildOf(FMassChunkFragment::StaticStruct()))
			{
				ChunkFragments.Add(*Struct);
			}
			// Check if the struct is a subclass of FMassSharedFragment
			else if (Struct->IsChildOf(FMassSharedFragment::StaticStruct()))
			{
				SharedFragments.Add(*Struct);
			}
			// Check if the struct is a subclass of FMassConstSharedFragment
			else if (Struct->IsChildOf(FMassConstSharedFragment::StaticStruct()))
			{
				ConstSharedFragments.Add(*Struct);
			}
		}

		// Construct the final Archetype Composition Descriptor using all collected bitsets
		OutComposition = FMassArchetypeCompositionDescriptor
		(
			MoveTemp(Fragments),
			MoveTemp(Tags),
			MoveTemp(ChunkFragments),
			MoveTemp(SharedFragments),
			MoveTemp(ConstSharedFragments)
		);
	}
};

/**
 * A Blueprint-accessible wrapper for defining entity templates in the editor.
 * This struct allows designers to compose entity templates by adding tags and fragments
 * with their initial values, which can then be converted to FMassEntityTemplateData in C++.
 */
USTRUCT(BlueprintType)
struct MASSAPI_API FEntityTemplate
{
	GENERATED_BODY()

public:

	/** List of tags to add to the entity. Since tags have no data, you just need to add an entry of the desired tag type. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Template", meta = (MASS_TAG_STRUCT, Tooltip = "List of tags to add to the entity. Since tags have no data, you just need to add an entry of the desired tag type."))
	TArray<FInstancedStruct> Tags;

	/** List of fragments with their initial values. Only structs derived from FMassFragment are allowed. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Template", meta = (MASS_FRAGMENT_STRUCT, Tooltip = "List of fragments with their initial values. Only structs derived from FMassFragment are allowed."))
	TArray<FInstancedStruct> Fragments;

	/** List of mutable shared fragments with their initial values. Only structs derived from FMassSharedFragment are allowed. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Template", meta = (MASS_SHARED_FRAGMENT_STRUCT, Tooltip = "List of mutable shared fragments with their initial values. Only structs derived from FMassSharedFragment are allowed."))
	TArray<FInstancedStruct> MutableSharedFragments;

	/** List of constant shared fragments with their initial values. Only structs derived from FMassConstSharedFragment are allowed. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Template", meta = (MASS_ENTITY_STRUCT, Tooltip = "List of constant shared fragments with their initial values. Only structs derived from FMassConstSharedFragment are allowed."))
	TArray<FInstancedStruct> ConstSharedFragments;

	/**
	 * Gathers the data from this struct and populates an FMassEntityTemplateData instance.
	 * This function should be called from C++ to convert the Blueprint-defined template into
	 * a format that the MassEntityManager can use to create entities.
	 * @param OutTemplateData The FMassEntityTemplateData to populate.
	 * @param EntityManager A reference to the active FMassEntityManager, required for creating shared fragment instances.
	 */
	void GetTemplateData(FMassEntityTemplateData& OutTemplateData, FMassEntityManager& EntityManager) const;
};

/**
 * A Blueprint-accessible wrapper for FMassEntityTemplateData.
 * This struct acts as a handle to the underlying template data, allowing it
 * to be created and modified via Blueprint nodes before being used to build entities.
 */
USTRUCT(BlueprintType)
struct MASSAPI_API FEntityTemplateData
{
	GENERATED_BODY()

public:
	FEntityTemplateData()
		: DataPtr(nullptr)
	{
	}

	FEntityTemplateData(TSharedPtr<FMassEntityTemplateData> InData)
		: DataPtr(InData)
	{
	}

	/** Checks if the handle points to valid data */
	bool IsValid() const { return DataPtr.IsValid(); }

	/** Provides direct access to the underlying FMassEntityTemplateData */
	FMassEntityTemplateData* Get() const { return DataPtr.Get(); }

private:
	// Use a shared pointer to manage the lifetime of the native FMassEntityTemplateData object.
	TSharedPtr<FMassEntityTemplateData> DataPtr;
};


