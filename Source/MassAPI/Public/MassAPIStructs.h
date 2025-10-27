#pragma once

#include "MassEntityTypes.h"
#include "MassEntityManager.h"
#include "MassEntityTemplate.h"
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

	// Blueprint-accessible entity index
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	int32 Index = 0;

	// Blueprint-accessible serial number
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
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
 * FEntityQuery allows for individual entity fragment and tag composition matching
 */
USTRUCT(BlueprintType)
struct MASSAPI_API FEntityQuery
{
	GENERATED_BODY()

public:
	// ----------- Constructors -----------

	/** Default constructor */
	FEntityQuery()
	{
		bIsAllCompDirty = true;
		bIsAnyCompDirty = true;
		bIsNoneCompDirty = true;
	}

	/** Constructor with cache control */
	FEntityQuery(bool bUpdateCache)
	{
		bIsAllCompDirty = bUpdateCache;
		bIsAnyCompDirty = bUpdateCache;
		bIsNoneCompDirty = bUpdateCache;
	}

	/** Convenient C++ constructor */
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
		UpdateCache();
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
		, bIsAllCompDirty(bUpdateCache)
		, bIsAnyCompDirty(bUpdateCache)
		, bIsNoneCompDirty(bUpdateCache)
	{
		UpdateCache();
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

	// ----------- Core Data UPROPERTY -----------

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "Entity traits: AND, the entity's Mass components must match this list completely."))
	TArray<TObjectPtr<UScriptStruct>> AllList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "Entity traits: OR, the entity's Mass components must have at least one from this list."))
	TArray<TObjectPtr<UScriptStruct>> AnyList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "Entity traits: NOT, the entity's Mass components must not match this list at all."))
	TArray<TObjectPtr<UScriptStruct>> NoneList;

	// ----------- Cached Descriptors -----------

private:
	mutable FMassArchetypeCompositionDescriptor AllComposition;
	mutable FMassArchetypeCompositionDescriptor AnyComposition;
	mutable FMassArchetypeCompositionDescriptor NoneComposition;

	mutable bool bIsAllCompDirty = false;
	mutable bool bIsAnyCompDirty = false;
	mutable bool bIsNoneCompDirty = false;

public:
	// ----------- Public Interface -----------

	const FMassArchetypeCompositionDescriptor& GetAllComposition() const
	{
		if (bIsAllCompDirty)
		{
			BuildCompositionFromStructs(AllComposition, AllList);
			bIsAllCompDirty = false;
		}
		return AllComposition;
	}

	const FMassArchetypeCompositionDescriptor& GetAnyComposition() const
	{
		if (bIsAnyCompDirty)
		{
			BuildCompositionFromStructs(AnyComposition, AnyList);
			bIsAnyCompDirty = false;
		}
		return AnyComposition;
	}

	const FMassArchetypeCompositionDescriptor& GetNoneComposition() const
	{
		if (bIsNoneCompDirty)
		{
			BuildCompositionFromStructs(NoneComposition, NoneList);
			bIsNoneCompDirty = false;
		}
		return NoneComposition;
	}

	FORCEINLINE void MarkCacheDirty()
	{
		bIsAllCompDirty = true;
		bIsAnyCompDirty = true;
		bIsNoneCompDirty = true;
	}

private:
	// ----------- Private Helper Functions -----------

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
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mass|Template", meta = (BaseStruct = "MassTag", Tooltip = "List of tags to add to the entity. Since tags have no data, you just need to add an entry of the desired tag type."))
	TArray<FInstancedStruct> Tags;

	/** List of fragments with their initial values. Only structs derived from FMassFragment are allowed. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mass|Template", meta = (BaseStruct = "MassFragment", Tooltip = "List of fragments with their initial values. Only structs derived from FMassFragment are allowed."))
	TArray<FInstancedStruct> Fragments;

	/** List of mutable shared fragments with their initial values. Only structs derived from FMassSharedFragment are allowed. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mass|Template", meta = (BaseStruct = "MassSharedFragment", Tooltip = "List of mutable shared fragments with their initial values. Only structs derived from FMassSharedFragment are allowed."))
	TArray<FInstancedStruct> MutableSharedFragments;

	/** List of constant shared fragments with their initial values. Only structs derived from FMassConstSharedFragment are allowed. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mass|Template", meta = (BaseStruct = "MassConstSharedFragment", Tooltip = "List of constant shared fragments with their initial values. Only structs derived from FMassConstSharedFragment are allowed."))
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
