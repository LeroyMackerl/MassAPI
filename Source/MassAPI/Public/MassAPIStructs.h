/*
* MassAPI
* Created: 2025
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "MassAPIVersion.h"
#include "MassEntityTypes.h"
#include "MassEntityManager.h"
#include "MassEntityTemplate.h"
#include "MassAPIEnums.h"
#include "MassEntityQuery.h"
#include <atomic>
#include "MassAPIFlagSettings.h"
#include "MassAPIStructs.generated.h"


/**
 * FEntityHandle is a blueprintable wrapper of MassEntityHandle
 */
USTRUCT(BlueprintType)
struct MASSAPI_API FEntityHandle
{
	GENERATED_BODY()

public:
	FEntityHandle() = default;
	FORCEINLINE FEntityHandle(const FMassEntityHandle& MassHandle) : Index(MassHandle.Index), Serial(MassHandle.SerialNumber) {}

	FORCEINLINE operator FMassEntityHandle() const { return FMassEntityHandle(Index, Serial); }
	FORCEINLINE bool IsSet() const { return Index != 0 && Serial != 0; }
	FORCEINLINE bool IsEmptyHandle() const { return Index != 0 && Serial != 0; }
	FORCEINLINE void Reset() { Index = 0; Serial = 0; }

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "MassAPI|EntityHandle")
	int32 Index = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "MassAPI|EntityHandle")
	int32 Serial = 0;

	FORCEINLINE bool operator==(const FEntityHandle& Other) const { return Index == Other.Index && Serial == Other.Serial; }
	FORCEINLINE bool operator!=(const FEntityHandle& Other) const { return !(*this == Other); }
	FORCEINLINE friend uint32 GetTypeHash(const FEntityHandle& Handle) { return HashCombine(GetTypeHash(Handle.Index), GetTypeHash(Handle.Serial)); }
};

/**
 * (New) Stores dynamic flags without causing Archetype migration.
 * Uses two int64 bitmasks for 128 flags total (Flags for 0-63, FlagsHigh for 64-127).
 * Thread-safe implementation using atomic spinlock.
 */
USTRUCT(BlueprintType)
struct MASSAPI_API FEntityFlagFragment : public FMassFragment
{
	GENERATED_BODY()

private:
	// The spinlock flag, copied from FStatistics implementation
	mutable std::atomic<bool> LockFlag{ false };

public:
	/** Low flags (0-63) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Flags")
	int64 Flags = 0;

	/** High flags (64-127) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Flags")
	int64 FlagsHigh = 0;

	// --- Constructors & Operators (Required for std::atomic) ---

	FEntityFlagFragment() {};

	FEntityFlagFragment(const FEntityFlagFragment& Other)
	{
		Flags = Other.Flags;
		FlagsHigh = Other.FlagsHigh;
	}

	FEntityFlagFragment& operator=(const FEntityFlagFragment& Other)
	{
		if (this != &Other)
		{
			Flags = Other.Flags;
			FlagsHigh = Other.FlagsHigh;
		}
		return *this;
	}

	// --- Locking Mechanics ---

	void Lock() const
	{
		while (LockFlag.exchange(true, std::memory_order_acquire));
	}

	void Unlock() const
	{
		LockFlag.store(false, std::memory_order_release);
	}

	// --- Helper Methods for 128-bit Flag Support ---

	/** Returns true if the flag index is >= 64 (stored in FlagsHigh) */
	FORCEINLINE static bool IsHighFlag(const EEntityFlags Flag)
	{
		return static_cast<uint8>(Flag) >= 64;
	}

	/** Returns the local bit index (0-63) within the appropriate int64 */
	FORCEINLINE static uint8 GetLocalBitIndex(const EEntityFlags Flag)
	{
		return static_cast<uint8>(Flag) & 63;  // Equivalent to % 64
	}

	// --- Flag Operations ---

	/**
	 * (New) Checks if this Fragment has a specific flag set.
	 * Note: This is a read operation and currently technically lock-free.
	 */
	FORCEINLINE bool HasFlag(const EEntityFlags Flag) const
	{
		if (Flag >= EEntityFlags::EEntityFlags_MAX) return false;
		const uint8 LocalIndex = GetLocalBitIndex(Flag);
		const int64 Mask = (1LL << LocalIndex);
		return IsHighFlag(Flag) ? (FlagsHigh & Mask) != 0 : (Flags & Mask) != 0;
	}

	/**
	 * (New) Sets a specific flag on this Fragment.
	 * Thread-safe: Uses Lock/Unlock, but checks first to avoid unnecessary locking.
	 */
	FORCEINLINE void SetFlag(const EEntityFlags Flag)
	{
		if (Flag >= EEntityFlags::EEntityFlags_MAX) return;

		// OPTIMIZATION: If the flag is already set, do not wait for a lock.
		if (HasFlag(Flag))
		{
			return;
		}

		Lock(); // Enter Critical Section

		const uint8 LocalIndex = GetLocalBitIndex(Flag);
		const int64 Mask = (1LL << LocalIndex);
		if (IsHighFlag(Flag))
		{
			FlagsHigh |= Mask;
		}
		else
		{
			Flags |= Mask;
		}

		Unlock(); // Exit Critical Section
	}

	/**
	 * (New) Clears a specific flag on this Fragment.
	 * Thread-safe: Uses Lock/Unlock, but checks first to avoid unnecessary locking.
	 */
	FORCEINLINE void ClearFlag(const EEntityFlags Flag)
	{
		if (Flag >= EEntityFlags::EEntityFlags_MAX) return;

		// OPTIMIZATION: If the flag is already clear, do not wait for a lock.
		if (!HasFlag(Flag))
		{
			return;
		}

		Lock(); // Enter Critical Section

		const uint8 LocalIndex = GetLocalBitIndex(Flag);
		const int64 Mask = (1LL << LocalIndex);
		if (IsHighFlag(Flag))
		{
			FlagsHigh &= ~Mask;
		}
		else
		{
			Flags &= ~Mask;
		}

		Unlock(); // Exit Critical Section
	}

	/**
	 * (New) Sets or Clears a specific flag based on a boolean value.
	 * Thread-safe: Uses Lock/Unlock, but checks first to avoid unnecessary locking.
	 */
	FORCEINLINE void SetFlagValue(const EEntityFlags Flag, const bool bValue)
	{
		if (Flag >= EEntityFlags::EEntityFlags_MAX) return;

		// OPTIMIZATION: If the current state already matches bValue, do nothing.
		if (HasFlag(Flag) == bValue)
		{
			return;
		}

		Lock(); // Enter Critical Section

		const uint8 LocalIndex = GetLocalBitIndex(Flag);
		const int64 Mask = (1LL << LocalIndex);
		if (bValue)
		{
			if (IsHighFlag(Flag))
			{
				FlagsHigh |= Mask;
			}
			else
			{
				Flags |= Mask;
			}
		}
		else
		{
			if (IsHighFlag(Flag))
			{
				FlagsHigh &= ~Mask;
			}
			else
			{
				Flags &= ~Mask;
			}
		}

		Unlock(); // Exit Critical Section
	}

	// --- FName-based flag operations | 基于FName的旗标操作 ---

	FORCEINLINE bool HasFlagByName(const FName FlagName) const
	{
		const EEntityFlags Resolved = UMassAPIFlagSettings::ResolveFlag(FlagName);
		return (Resolved != EEntityFlags::EEntityFlags_MAX) && HasFlag(Resolved);
	}

	FORCEINLINE void SetFlagByName(const FName FlagName)
	{
		const EEntityFlags Resolved = UMassAPIFlagSettings::ResolveFlag(FlagName);
		if (Resolved != EEntityFlags::EEntityFlags_MAX)
		{
			SetFlag(Resolved);
		}
	}

	FORCEINLINE void ClearFlagByName(const FName FlagName)
	{
		const EEntityFlags Resolved = UMassAPIFlagSettings::ResolveFlag(FlagName);
		if (Resolved != EEntityFlags::EEntityFlags_MAX)
		{
			ClearFlag(Resolved);
		}
	}

	FORCEINLINE void SetFlagValueByName(const FName FlagName, const bool bValue)
	{
		const EEntityFlags Resolved = UMassAPIFlagSettings::ResolveFlag(FlagName);
		if (Resolved != EEntityFlags::EEntityFlags_MAX)
		{
			SetFlagValue(Resolved, bValue);
		}
	}
};

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
template<>
struct TMassFragmentTraits<FEntityFlagFragment>
{
	static constexpr bool AuthorAcceptsItsNotTriviallyCopyable = true;
};
#endif

/**
 * FEntityQuery allows for individual entity fragment and tag composition matching
 */
USTRUCT(BlueprintType)
struct MASSAPI_API FEntityQuery
{
	GENERATED_BODY()

public:
	// ----------- Constructors -----------

	FEntityQuery()
	{
		MarkCacheDirty();
	}

	FEntityQuery(bool bUpdateCache)
	{
		if (bUpdateCache) MarkCacheDirty();
		else ClearDirtyFlags();
	}

	FEntityQuery(
		TArray<TObjectPtr<UScriptStruct>> InAll,
		TArray<TObjectPtr<UScriptStruct>> InAny,
		TArray<TObjectPtr<UScriptStruct>> InNone,
		bool bUpdateCache = true
	)
		: AllList(MoveTemp(InAll))
		, AnyList(MoveTemp(InAny))
		, NoneList(MoveTemp(InNone))
	{
		if (bUpdateCache) { MarkCacheDirty(); UpdateCache(); }
		else ClearDirtyFlags();
	}

	// ----------- Fluent API -----------

	template<typename... TArgs>
	FORCEINLINE FEntityQuery& All()&
	{
		(CheckType<TArgs>(), ...);
		(AllList.Add(TArgs::StaticStruct()), ...);
		MarkCacheDirty();
		return *this;
	}
	template<typename... TArgs>
	FORCEINLINE FEntityQuery&& All()&&
	{
		MarkCacheDirty();
		return MoveTemp(this->All<TArgs...>());
	}

	template<typename... TArgs>
	FORCEINLINE FEntityQuery& Any()&
	{
		(CheckType<TArgs>(), ...);
		(AnyList.Add(TArgs::StaticStruct()), ...);
		MarkCacheDirty();
		return *this;
	}
	template<typename... TArgs>
	FORCEINLINE FEntityQuery&& Any()&&
	{
		MarkCacheDirty();
		return MoveTemp(this->Any<TArgs...>());
	}

	template<typename... TArgs>
	FORCEINLINE FEntityQuery& None()&
	{
		(CheckType<TArgs>(), ...);
		(NoneList.Add(TArgs::StaticStruct()), ...);
		MarkCacheDirty();
		return *this;
	}
	template<typename... TArgs>
	FORCEINLINE FEntityQuery&& None()&&
	{
		MarkCacheDirty();
		return MoveTemp(this->None<TArgs...>());
	}

	// ----------- UPROPERTIES -----------

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Query")
	TArray<TObjectPtr<UScriptStruct>> AllList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Query")
	TArray<TObjectPtr<UScriptStruct>> AnyList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Query")
	TArray<TObjectPtr<UScriptStruct>> NoneList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Flags")
	TArray<FName> AllFlagsList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Flags")
	TArray<FName> AnyFlagsList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Flags")
	TArray<FName> NoneFlagsList;

private:
	// Composition Descriptors (RESTORED)
	mutable FMassArchetypeCompositionDescriptor AllComposition;
	mutable FMassArchetypeCompositionDescriptor AnyComposition;
	mutable FMassArchetypeCompositionDescriptor NoneComposition;

	// Bitmask Cache (Low flags 0-63)
	mutable int64 AllFlagsBitmask_Cache = 0;
	mutable int64 AnyFlagsBitmask_Cache = 0;
	mutable int64 NoneFlagsBitmask_Cache = 0;

	// Bitmask Cache (High flags 64-127)
	mutable int64 AllFlagsBitmaskHigh_Cache = 0;
	mutable int64 AnyFlagsBitmaskHigh_Cache = 0;
	mutable int64 NoneFlagsBitmaskHigh_Cache = 0;

	// Dirty Flags
	mutable bool bIsAllCompDirty = false;
	mutable bool bIsAnyCompDirty = false;
	mutable bool bIsNoneCompDirty = false;
	mutable bool bIsFlagsCacheDirty = false;

public:
	// ----------- Public Interface -----------

	// [RESTORED] Composition Accessors
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

	// Flag Accessors (Low flags 0-63)
	int64 GetAllFlagsBitmask() const { if (bIsFlagsCacheDirty) BuildFlagsCache(); return AllFlagsBitmask_Cache; }
	int64 GetAnyFlagsBitmask() const { if (bIsFlagsCacheDirty) BuildFlagsCache(); return AnyFlagsBitmask_Cache; }
	int64 GetNoneFlagsBitmask() const { if (bIsFlagsCacheDirty) BuildFlagsCache(); return NoneFlagsBitmask_Cache; }

	// Flag Accessors (High flags 64-127)
	int64 GetAllFlagsBitmaskHigh() const { if (bIsFlagsCacheDirty) BuildFlagsCache(); return AllFlagsBitmaskHigh_Cache; }
	int64 GetAnyFlagsBitmaskHigh() const { if (bIsFlagsCacheDirty) BuildFlagsCache(); return AnyFlagsBitmaskHigh_Cache; }
	int64 GetNoneFlagsBitmaskHigh() const { if (bIsFlagsCacheDirty) BuildFlagsCache(); return NoneFlagsBitmaskHigh_Cache; }

	/** * Returns a native FMassEntityQuery bound to the provided EntityManager.
	 * Note: We cannot cache the bound query itself because the Manager can change,
	 * but we reuse the struct lists to populate it efficiently.
	 */
	FMassEntityQuery GetNativeQuery(const TSharedPtr<FMassEntityManager>& EntityManager) const
	{
		// 1. Create Bound Query
		FMassEntityQuery NativeQuery(EntityManager);

		// 2. Populate Requirements
		ConfigureQuery(NativeQuery);

		return NativeQuery;
	}

	FORCEINLINE void MarkCacheDirty()
	{
		bIsAllCompDirty = true;
		bIsAnyCompDirty = true;
		bIsNoneCompDirty = true;
		bIsFlagsCacheDirty = true;
	}

private:
	// ----------- Private Helpers -----------

	void ClearDirtyFlags()
	{
		bIsAllCompDirty = false;
		bIsAnyCompDirty = false;
		bIsNoneCompDirty = false;
		bIsFlagsCacheDirty = false;
	}

	void UpdateCache() const
	{
		if (bIsAllCompDirty) { BuildCompositionFromStructs(AllComposition, AllList); bIsAllCompDirty = false; }
		if (bIsAnyCompDirty) { BuildCompositionFromStructs(AnyComposition, AnyList); bIsAnyCompDirty = false; }
		if (bIsNoneCompDirty) { BuildCompositionFromStructs(NoneComposition, NoneList); bIsNoneCompDirty = false; }
		if (bIsFlagsCacheDirty) { BuildFlagsCache(); }
	}

	/** Helper to populate a native query from our lists */
	void ConfigureQuery(FMassEntityQuery& OutQuery) const
	{
		auto AddReqs = [&](const TArray<TObjectPtr<UScriptStruct>>& List, EMassFragmentPresence TagP, EMassFragmentPresence FragP, EMassFragmentAccess Access)
			{
				for (const TObjectPtr<UScriptStruct>& Struct : List)
				{
					if (!Struct) continue;
					if (Struct->IsChildOf(FMassTag::StaticStruct())) { OutQuery.AddTagRequirement(*Struct, TagP); }
					else if (Struct->IsChildOf(FMassSharedFragment::StaticStruct())) { OutQuery.AddSharedRequirement(Struct, Access, FragP); }
					else if (Struct->IsChildOf(FMassConstSharedFragment::StaticStruct())) { OutQuery.AddConstSharedRequirement(Struct, FragP); }
					else if (Struct->IsChildOf(FMassChunkFragment::StaticStruct())) { OutQuery.AddChunkRequirement(Struct, Access, FragP); }
					else if (Struct->IsChildOf(FMassFragment::StaticStruct())) { OutQuery.AddRequirement(Struct, Access, FragP); }
				}
			};

		AddReqs(AllList, EMassFragmentPresence::All, EMassFragmentPresence::All, EMassFragmentAccess::ReadOnly);
		AddReqs(AnyList, EMassFragmentPresence::Any, EMassFragmentPresence::Any, EMassFragmentAccess::ReadOnly);
		AddReqs(NoneList, EMassFragmentPresence::None, EMassFragmentPresence::None, EMassFragmentAccess::None);
	}

	void BuildFlagsCache() const
	{
		// Reset all caches
		AllFlagsBitmask_Cache = 0; AnyFlagsBitmask_Cache = 0; NoneFlagsBitmask_Cache = 0;
		AllFlagsBitmaskHigh_Cache = 0; AnyFlagsBitmaskHigh_Cache = 0; NoneFlagsBitmaskHigh_Cache = 0;

		// Resolve FName flags via settings registry | 通过设置注册表解析 FName 旗标
		if (const UMassAPIFlagSettings* Settings = GetDefault<UMassAPIFlagSettings>())
		{
			auto SetFlagBit = [&](int64& LowMask, int64& HighMask, const FName& FlagName)
			{
				if (const EEntityFlags* Flag = Settings->FlagRegistry.Find(FlagName))
				{
					const uint8 Index = static_cast<uint8>(*Flag);
					if (Index >= 64) { HighMask |= (1LL << (Index - 64)); }
					else { LowMask |= (1LL << Index); }
				}
			};
			for (const FName& FlagName : AllFlagsList) { SetFlagBit(AllFlagsBitmask_Cache, AllFlagsBitmaskHigh_Cache, FlagName); }
			for (const FName& FlagName : AnyFlagsList) { SetFlagBit(AnyFlagsBitmask_Cache, AnyFlagsBitmaskHigh_Cache, FlagName); }
			for (const FName& FlagName : NoneFlagsList) { SetFlagBit(NoneFlagsBitmask_Cache, NoneFlagsBitmaskHigh_Cache, FlagName); }
		}

		bIsFlagsCacheDirty = false;
	}

	void BuildCompositionFromStructs(FMassArchetypeCompositionDescriptor& OutComp, const TArray<TObjectPtr<UScriptStruct>>& Structs) const
	{
		FMassFragmentBitSet F; FMassTagBitSet T; FMassChunkFragmentBitSet C; FMassSharedFragmentBitSet S; FMassConstSharedFragmentBitSet CS;
		for (const TObjectPtr<UScriptStruct>& St : Structs)
		{
			if (!IsValid(St.Get())) continue;
			if (St->IsChildOf(FMassFragment::StaticStruct())) BIT_SET_ADD(F, St);
			else if (St->IsChildOf(FMassTag::StaticStruct())) BIT_SET_ADD(T, St);
			else if (St->IsChildOf(FMassChunkFragment::StaticStruct())) BIT_SET_ADD(C, St);
			else if (St->IsChildOf(FMassSharedFragment::StaticStruct())) BIT_SET_ADD(S, St);
			else if (St->IsChildOf(FMassConstSharedFragment::StaticStruct())) BIT_SET_ADD(CS, St);
		}
		OutComp = FMassArchetypeCompositionDescriptor(MoveTemp(F), MoveTemp(T), MoveTemp(C), MoveTemp(S), MoveTemp(CS));
	}

	template<typename T>
	static constexpr void CheckType()
	{
		static_assert(
			TIsDerivedFrom<T, FMassFragment>::IsDerived || TIsDerivedFrom<T, FMassTag>::IsDerived ||
			TIsDerivedFrom<T, FMassChunkFragment>::IsDerived || TIsDerivedFrom<T, FMassSharedFragment>::IsDerived ||
			TIsDerivedFrom<T, FMassConstSharedFragment>::IsDerived,
			"Template arguments must be derived from FMassFragment, FMassTag, etc."
			);
	}
};

// ----------- Fluent Query Builder | 流式查询构建器 -----------

// Compile-time access tags — short, collision-resistant | 编译期读写标签，简短防冲突
template<EMassFragmentAccess Val>
struct FAccessTag { static constexpr EMassFragmentAccess Value = Val; };

inline constexpr FAccessTag<EMassFragmentAccess::ReadOnly>  MARO;  // MassAPI ReadOnly
inline constexpr FAccessTag<EMassFragmentAccess::ReadWrite> MARW;  // MassAPI ReadWrite

/**
 * Fluent API for configuring FMassEntityQuery from processor ConfigureQueries.
 * Groups fragments by (Presence, Access) with variadic template packs,
 * auto-detects Tags/Shared/ConstShared for type-safe dispatch.
 *
 * Usage | 用法:
 *   FEntityQueryBuilder(EntityQuery)
 *       .All<FAgentTag, FAITag>()                 // Tags: access ignored | 标签：忽略读写
 *       .All<FLocating, FRotating>(MARW)          // ReadWrite
 *       .All<FScaling, FCollider>(MARO)           // ReadOnly
 *       .RegisterWithProcessor(*this);
 */
struct FEntityQueryBuilder
{
	FMassEntityQuery& Query;

	explicit FEntityQueryBuilder(FMassEntityQuery& InQuery) : Query(InQuery) {}

	template<typename... TArgs, typename TAccess = decltype(MARO)>
	FEntityQueryBuilder& All(TAccess = MARO)
	{
		(AddSingle<TArgs, TAccess::Value>(EMassFragmentPresence::All), ...);
		return *this;
	}

	template<typename... TArgs, typename TAccess = decltype(MARO)>
	FEntityQueryBuilder& Any(TAccess = MARO)
	{
		(AddSingle<TArgs, TAccess::Value>(EMassFragmentPresence::Any), ...);
		return *this;
	}

	template<typename... TArgs, typename TAccess = decltype(MARO)>
	FEntityQueryBuilder& None(TAccess = MARO)
	{
		(AddSingle<TArgs, TAccess::Value>(EMassFragmentPresence::None), ...);
		return *this;
	}

	template<typename... TArgs, typename TAccess = decltype(MARO)>
	FEntityQueryBuilder& Optional(TAccess = MARO)
	{
		(AddSingle<TArgs, TAccess::Value>(EMassFragmentPresence::Optional), ...);
		return *this;
	}

	void RegisterWithProcessor(UMassProcessor& Processor)
	{
		Query.RegisterWithProcessor(Processor);
	}

private:
	template<typename T, EMassFragmentAccess Access>
	void AddSingle(EMassFragmentPresence Presence)
	{
		if constexpr (TIsDerivedFrom<T, FMassTag>::IsDerived)
		{
			// Tag — silently ignore Access | 标签 — 静默忽略读写
			Query.AddTagRequirement<T>(Presence);
		}
		else if constexpr (TIsDerivedFrom<T, FMassConstSharedFragment>::IsDerived)
		{
			// ConstSharedFragment + ReadWrite = compile-time error | 常量共享片段 + 写入 = 编译期报错
			static_assert(Access != EMassFragmentAccess::ReadWrite,
				"ConstSharedFragment cannot be ReadWrite. Omit () or use ReadOnly. | 常量共享片段不支持 ReadWrite。请省略 () 或使用 ReadOnly。");
			Query.AddConstSharedRequirement<T>(Presence);
		}
		else if constexpr (TIsDerivedFrom<T, FMassSharedFragment>::IsDerived)
		{
			Query.AddSharedRequirement<T>(Access, Presence);
		}
		else if constexpr (TIsDerivedFrom<T, FMassChunkFragment>::IsDerived)
		{
			Query.AddChunkRequirement<T>(Access, Presence);
		}
		else
		{
			Query.AddRequirement<T>(Access, Presence);
		}
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
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Template", meta = (BaseStruct = "/Script/MassEntity.MassTag", Tooltip = "List of tags to add to the entity. Since tags have no data, you just need to add an entry of the desired tag type."))
	TArray<FInstancedStruct> Tags;

	/** List of fragments with their initial values. Only structs derived from FMassFragment are allowed. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Template", meta = (BaseStruct = "/Script/MassEntity.MassFragment", Tooltip = "List of fragments with their initial values. Only structs derived from FMassFragment are allowed."))
	TArray<FInstancedStruct> Fragments;

	/** List of mutable shared fragments with their initial values. Only structs derived from FMassSharedFragment are allowed. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Template", meta = (BaseStruct = "/Script/MassEntity.MassSharedFragment", Tooltip = "List of mutable shared fragments with their initial values. Only structs derived from FMassSharedFragment are allowed."))
	TArray<FInstancedStruct> MutableSharedFragments;

	/** List of constant shared fragments with their initial values. Only structs derived from FMassConstSharedFragment are allowed. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Template", meta = (BaseStruct = "/Script/MassEntity.MassConstSharedFragment", Tooltip = "List of constant shared fragments with their initial values. Only structs derived from FMassConstSharedFragment are allowed."))
	TArray<FInstancedStruct> ConstSharedFragments;

	/** List of initial flags to set on the entity. These will be packed into an FEntityFlagFragment. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Template", meta = (Tooltip = "List of initial flags to set on the entity. These will be converted into an FEntityFlagFragment bitmask."))
	TArray<EEntityFlags> Flags;

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
