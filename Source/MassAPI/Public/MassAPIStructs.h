/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#pragma once

#include "MassEntityTypes.h"
#include "MassEntityManager.h"
#include "MassEntityTemplate.h"
#include "MassAPIEnums.h" // 包含新的枚举头文件
#include "MassEntityQuery.h"
#include "MassAPIStructs.generated.h" 

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

	/** Resets the handle to an invalid state (Index 0, Serial 0) */
	FORCEINLINE void Reset()
	{
		Index = 0;
		Serial = 0;
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
 * 蓝图将使用 MassAPIFuncLib 中的辅助函数来操作这个位掩码。
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
    TArray<EEntityFlags> AllFlagsList;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Flags")
    TArray<EEntityFlags> AnyFlagsList;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|Flags")
    TArray<EEntityFlags> NoneFlagsList;

    // ----------- Caching Logic -----------

private:
    // Composition Descriptors (RESTORED)
    mutable FMassArchetypeCompositionDescriptor AllComposition;
    mutable FMassArchetypeCompositionDescriptor AnyComposition;
    mutable FMassArchetypeCompositionDescriptor NoneComposition;

    // Bitmask Cache
    mutable int64 AllFlagsBitmask_Cache = 0;
    mutable int64 AnyFlagsBitmask_Cache = 0;
    mutable int64 NoneFlagsBitmask_Cache = 0;

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

    // Flag Accessors
    int64 GetAllFlagsBitmask() const { if (bIsFlagsCacheDirty) BuildFlagsCache(); return AllFlagsBitmask_Cache; }
    int64 GetAnyFlagsBitmask() const { if (bIsFlagsCacheDirty) BuildFlagsCache(); return AnyFlagsBitmask_Cache; }
    int64 GetNoneFlagsBitmask() const { if (bIsFlagsCacheDirty) BuildFlagsCache(); return NoneFlagsBitmask_Cache; }

    /** * [NEW] Returns a native FMassEntityQuery bound to the provided EntityManager.
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
        AllFlagsBitmask_Cache = 0; AnyFlagsBitmask_Cache = 0; NoneFlagsBitmask_Cache = 0;
        for (const EEntityFlags Flag : AllFlagsList) if (Flag < EEntityFlags::EEntityFlags_MAX) AllFlagsBitmask_Cache |= (1LL << (uint8)Flag);
        for (const EEntityFlags Flag : AnyFlagsList) if (Flag < EEntityFlags::EEntityFlags_MAX) AnyFlagsBitmask_Cache |= (1LL << (uint8)Flag);
        for (const EEntityFlags Flag : NoneFlagsList) if (Flag < EEntityFlags::EEntityFlags_MAX) NoneFlagsBitmask_Cache |= (1LL << (uint8)Flag);
        bIsFlagsCacheDirty = false;
    }

    void BuildCompositionFromStructs(FMassArchetypeCompositionDescriptor& OutComp, const TArray<TObjectPtr<UScriptStruct>>& Structs) const
    {
        FMassFragmentBitSet F; FMassTagBitSet T; FMassChunkFragmentBitSet C; FMassSharedFragmentBitSet S; FMassConstSharedFragmentBitSet CS;
        for (const TObjectPtr<UScriptStruct>& St : Structs)
        {
            if (!IsValid(St.Get())) continue;
            if (St->IsChildOf(FMassFragment::StaticStruct())) F.Add(*St);
            else if (St->IsChildOf(FMassTag::StaticStruct())) T.Add(*St);
            else if (St->IsChildOf(FMassChunkFragment::StaticStruct())) C.Add(*St);
            else if (St->IsChildOf(FMassSharedFragment::StaticStruct())) S.Add(*St);
            else if (St->IsChildOf(FMassConstSharedFragment::StaticStruct())) CS.Add(*St);
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
