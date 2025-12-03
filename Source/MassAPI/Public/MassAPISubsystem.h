/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "MassSubsystemBase.h"
#include "MassAPIStructs.h" // 已包含 FEntityFlagFragment 和 FEntityQuery
#include "MassAPIEnums.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "MassEntityView.h"
#include "MassEntityTemplate.h"
#include "MassCommands.h"
#include "MassCommandBuffer.h"
#include "MassExecutionContext.h"
#include "MassAPISubsystem.generated.h"


/**
 * UMassAPISubsystem provides convenient API extensions for MassEntity framework
 * This subsystem wraps common entity operations for easier access
 */
UCLASS()
class MASSAPI_API UMassAPISubsystem : public UMassSubsystemBase
{
	GENERATED_BODY()


protected:

	// Subsystem lifecycle
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;


public:
	//-------------- Getting MassAPI ---------------

	/**
	 * Static getter for the subsystem (returns pointer)
	 * @param WorldContextObject Any UObject that can provide world context
	 * @return Pointer to MassAPISubsystem, nullptr if not available
	 */
	static UMassAPISubsystem* GetPtr(const UObject* WorldContextObject);

	/**
	 * Static getter for the subsystem (returns reference, will check validity)
	 * @param WorldContextObject Any UObject that can provide world context
	 * @return Reference to MassAPISubsystem
	 */
	static UMassAPISubsystem& GetRef(const UObject* WorldContextObject);

	/**
	 * Gets the entity manager, lazy-loading it if necessary.
	 * This is safe to call from any function, including const ones.
	 * @return A pointer to the FMassEntityManager, or nullptr if it cannot be found.
	 */
	FMassEntityManager* GetEntityManager() const;

	/**
	 * Gets the global command buffer from the entity manager.
	 * This is a convenience wrapper for GetEntityManager()->Defer()
	 * WARNING: Only use this outside of processor contexts!
	 * Inside processors, use Context.Defer() instead
	 * @return Reference to the global command buffer
	 */
	FORCEINLINE FMassCommandBuffer& Defer() const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available"));
		return Manager->Defer();
	}


	//----------------- Filters ---------------

	// A contains all of B
	FORCEINLINE static bool HasAll(const FMassArchetypeCompositionDescriptor& ThisComposition, const FMassArchetypeCompositionDescriptor& OtherComposition)
	{
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		return
			ThisComposition.GetFragments().HasAll(OtherComposition.GetFragments()) &&
			ThisComposition.GetTags().HasAll(OtherComposition.GetTags()) &&
			ThisComposition.GetChunkFragments().HasAll(OtherComposition.GetChunkFragments()) &&
			ThisComposition.GetSharedFragments().HasAll(OtherComposition.GetSharedFragments()) &&
			ThisComposition.GetConstSharedFragments().HasAll(OtherComposition.GetConstSharedFragments());
		#else
		return
			ThisComposition.Fragments.HasAll(OtherComposition.Fragments) &&
			ThisComposition.Tags.HasAll(OtherComposition.Tags) &&
			ThisComposition.ChunkFragments.HasAll(OtherComposition.ChunkFragments) &&
			ThisComposition.SharedFragments.HasAll(OtherComposition.SharedFragments) &&
			ThisComposition.ConstSharedFragments.HasAll(OtherComposition.ConstSharedFragments);
		#endif
	}

	// A contains any of B
	FORCEINLINE static bool HasAny(const FMassArchetypeCompositionDescriptor& ThisComposition, const FMassArchetypeCompositionDescriptor& OtherComposition)
	{
		#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		return
			ThisComposition.GetFragments().HasAny(OtherComposition.GetFragments()) ||
			ThisComposition.GetTags().HasAny(OtherComposition.GetTags()) ||
			ThisComposition.GetChunkFragments().HasAny(OtherComposition.GetChunkFragments()) ||
			ThisComposition.GetSharedFragments().HasAny(OtherComposition.GetSharedFragments()) ||
			ThisComposition.GetConstSharedFragments().HasAny(OtherComposition.GetConstSharedFragments());
		#else
		return
			ThisComposition.Fragments.HasAny(OtherComposition.Fragments) &&
			ThisComposition.Tags.HasAny(OtherComposition.Tags) &&
			ThisComposition.ChunkFragments.HasAny(OtherComposition.ChunkFragments) &&
			ThisComposition.SharedFragments.HasAny(OtherComposition.SharedFragments) &&
			ThisComposition.ConstSharedFragments.HasAny(OtherComposition.ConstSharedFragments);
		#endif
	}

	FORCEINLINE static bool MatchQueryAll(const FMassArchetypeCompositionDescriptor& Composition, const FEntityQuery& Query)
	{
		// AND condition: If AllList is empty, automatically passes. Otherwise, must have ALL.
		const bool AllResult = Query.AllList.IsEmpty() || HasAll(Composition, Query.GetAllComposition());

		return AllResult;
	}

	FORCEINLINE static bool MatchQueryAny(const FMassArchetypeCompositionDescriptor& Composition, const FEntityQuery& Query)
	{
		// OR condition: If AnyList is empty, automatically passes. Otherwise, must have ANY.
		const bool AnyResult = Query.AnyList.IsEmpty() || HasAny(Composition, Query.GetAnyComposition());

		return AnyResult;
	}

	FORCEINLINE static bool MatchQueryNone(const FMassArchetypeCompositionDescriptor& Composition, const FEntityQuery& Query)
	{
		// NOT condition: If NoneList is empty, automatically passes. Otherwise, must have NONE.
		const bool NoneResult = Query.NoneList.IsEmpty() || !HasAny(Composition, Query.GetNoneComposition());

		return NoneResult;
	}

	// Check if composition matches filter
	FORCEINLINE static bool MatchQuery(const FMassArchetypeCompositionDescriptor& Composition, const FEntityQuery& Query)
	{
		// AND condition: If AllList is empty, automatically passes. Otherwise, must have ALL.
		const bool AllResult = Query.AllList.IsEmpty() || HasAll(Composition, Query.GetAllComposition());
		// OR condition: If AnyList is empty, automatically passes. Otherwise, must have ANY.
		const bool AnyResult = Query.AnyList.IsEmpty() || HasAny(Composition, Query.GetAnyComposition());
		// NOT condition: If NoneList is empty, automatically passes. Otherwise, must have NONE.
		const bool NoneResult = Query.NoneList.IsEmpty() || !HasAny(Composition, Query.GetNoneComposition());

		return AllResult && AnyResult && NoneResult;
	}

	/** * 检查实体的 Composition (Tags / Fragments) 是否匹配查询
	 * @param Manager 实体管理器实例
	 * @param ArchetypeHandle 要检查的实体的原型句柄
	 * @param Query 要匹配的查询
	 * @return 如果 Composition 匹配则返回 true, 否则返回 false
	 */
	FORCEINLINE bool MatchQueryComposition(const FMassArchetypeHandle ArchetypeHandle, const FEntityQuery& Query) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available"));

		const FMassArchetypeCompositionDescriptor& Composition = Manager->GetArchetypeComposition(ArchetypeHandle);

		const bool AllResult = Query.AllList.IsEmpty() || HasAll(Composition, Query.GetAllComposition());
		const bool AnyResult = Query.AnyList.IsEmpty() || HasAny(Composition, Query.GetAnyComposition());
		const bool NoneResult = Query.NoneList.IsEmpty() || !HasAny(Composition, Query.GetNoneComposition());

		return (AllResult && AnyResult && NoneResult);
	}

	/** * 检查实体的 Flags 是否匹配查询
	 * @param Manager 实体管理器实例
	 * @param EntityHandle 要检查的实体的句柄
	 * @param Query 要匹配的查询
	 * @return 如果 Flags 匹配则返回 true, 否则返回 false
	 */
	FORCEINLINE bool MatchQueryFlag(const FEntityHandle EntityHandle, const FEntityQuery& Query) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available"));

		// 从查询中获取缓存的位掩码
		const int64 AllFlagsQuery = Query.GetAllFlagsBitmask();
		const int64 AnyFlagsQuery = Query.GetAnyFlagsBitmask();
		const int64 NoneFlagsQuery = Query.GetNoneFlagsBitmask();

		// 只有在有标志位查询时才执行检查
		if (AllFlagsQuery == 0 && AnyFlagsQuery == 0 && NoneFlagsQuery == 0)
		{
			return true; // 没有标志位查询，总是通过
		}

		// 尝试获取实体的 Flag Fragment
		const FEntityFlagFragment* FlagFragment = Manager->GetFragmentDataPtr<FEntityFlagFragment>(EntityHandle);

		// 如果实体没有 Flag Fragment，则认为它的标志位为 0
		const int64 EntityFlags = FlagFragment ? FlagFragment->Flags : 0;

		// 执行标志位检查
		const bool bAllFlags = (AllFlagsQuery == 0) || ((EntityFlags & AllFlagsQuery) == AllFlagsQuery);
		const bool bAnyFlags = (AnyFlagsQuery == 0) || ((EntityFlags & AnyFlagsQuery) != 0);
		const bool bNoneFlags = (NoneFlagsQuery == 0) || ((EntityFlags & NoneFlagsQuery) == 0);

		return (bAllFlags && bAnyFlags && bNoneFlags);
	}

	/**
	 * 检查实体是否完整匹配查询（包括 Composition 和 Flags）
	 * @param EntityHandle 要检查的实体的句柄
	 * @param Query 要匹配的查询
	 * @return 如果实体完整匹配查询则返回 true, 否则返回 false
	 */
	FORCEINLINE bool MatchQuery(const FEntityHandle EntityHandle, const FEntityQuery& Query) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available"));

		if (UNLIKELY(!Manager->IsEntityActive(EntityHandle))) return false;

		const FMassArchetypeHandle ArchetypeHandle = Manager->GetArchetypeForEntity(EntityHandle);
		if (UNLIKELY(!ArchetypeHandle.IsValid())) return false;

		// --- 1. 检查 Composition ---
		if (!MatchQueryComposition(ArchetypeHandle, Query))
		{
			return false;
		}

		// --- 2. 检查 Flags ---
		if (!MatchQueryFlag(EntityHandle, Query))
		{
			return false;
		}

		// 所有检查都通过
		return true;
	}


	//--------------- Entity Operations ---------------

	// Check if EntityHandle is valid, Runtime check
	FORCEINLINE bool IsValid(const FEntityHandle EntityHandle) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available"));
		return Manager->IsEntityValid(EntityHandle) && Manager->IsEntityBuilt(EntityHandle);
	}

	/**
	 * Reserves an entity handle without creating its data immediately.
	 * This is useful when you need an entity handle to reference before the entity is fully constructed.
	 * The reserved entity is inactive until you call BuildEntity on it.
	 * @return A reserved, inactive FMassEntityHandle. Returns an invalid handle if the EntityManager is not available.
	 */
	FORCEINLINE FMassEntityHandle ReserveEntity() const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available"));
		return Manager->ReserveEntity();
	}

	// Build entities using a template
	TArray<FMassEntityHandle> BuildEntities(int32 Quantity, FMassEntityTemplateData& TemplateData) const;

	/**
	 * Spawn entities with mixed tags, fragments, and shared fragments
	 * The function automatically distinguishes between different types
	 * @param Quantity Number of entities to spawn
	 * @param Args Mixed tags, fragments, and shared fragments
	 * @return Array of spawned entity handles
	 *
	 * Usage:
	 * BuildEntities(10, FEnemyTag{}, FHealthFragment{100.0f}, FTeamSharedFragment{1})
	 */
	template<typename... TArgs>
	TArray<FMassEntityHandle> BuildEntities(int32 Quantity, TArgs&&... Args) const
	{
		TArray<FMassEntityHandle> SpawnedEntities;
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available"));

		// Validate input
		if (Quantity <= 0)
		{
			return SpawnedEntities;
		}

		// Build the composition descriptor
		FMassFragmentBitSet Fragments;
		FMassTagBitSet Tags;
		FMassSharedFragmentBitSet SharedFragments;
		FMassConstSharedFragmentBitSet ConstSharedFragments;

		TArray<FInstancedStruct> FragmentInstances;
		FMassArchetypeSharedFragmentValues SharedFragmentValues;

		// Reserve space for fragment instances
		if constexpr (sizeof...(Args) > 0)
		{
			FragmentInstances.Reserve(sizeof...(Args));

			// Helper lambda to process each argument
			auto ProcessArg = [&](auto&& Arg)
				{
					using ArgType = std::decay_t<decltype(Arg)>;

					// Check if it's a Tag
					if constexpr (UE::Mass::CTag<ArgType>)
					{
						Tags.Add(*ArgType::StaticStruct());
					}
					// Check if it's a regular Fragment
					else if constexpr (UE::Mass::CFragment<ArgType>)
					{
						Fragments.Add(*ArgType::StaticStruct());
						FragmentInstances.Add(FInstancedStruct::Make(Forward<decltype(Arg)>(Arg)));
					}
					// Check if it's specifically a SharedFragment or ConstSharedFragment
					else if constexpr (UE::Mass::CSharedFragment<ArgType>)
					{
						SharedFragments.Add(*ArgType::StaticStruct());
						// Create or get the shared fragment instance
						const FSharedStruct& SharedStruct = Manager->GetOrCreateSharedFragment(Forward<decltype(Arg)>(Arg));
						SharedFragmentValues.Add(SharedStruct);
					}
					else if constexpr (UE::Mass::CConstSharedFragment<ArgType>)
					{
						ConstSharedFragments.Add(*ArgType::StaticStruct());
						// Create or get the const shared fragment instance
						const FConstSharedStruct& ConstSharedStruct = Manager->GetOrCreateConstSharedFragment(Forward<decltype(Arg)>(Arg));
						SharedFragmentValues.Add(ConstSharedStruct);
					}
					else
					{
						static_assert(UE::Mass::TAlwaysFalse<ArgType>,
							"Arguments must be MassTags, MassFragments, MassSharedFragments, or MassConstSharedFragments");
					}
				};

			// Process all arguments
			(ProcessArg(Forward<TArgs>(Args)), ...);
		}

		// Create the composition descriptor
		FMassArchetypeCompositionDescriptor Composition
		(
			MoveTemp(Fragments),
			MoveTemp(Tags),
			FMassChunkFragmentBitSet(),
			MoveTemp(SharedFragments),
			MoveTemp(ConstSharedFragments)
		);

		// Create the archetype
		FMassArchetypeHandle ArchetypeHandle = Manager->CreateArchetype(Composition);

		if (!ArchetypeHandle.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to create archetype for entity spawning"));
			return SpawnedEntities;
		}

		// Reserve space for entities
		SpawnedEntities.Reserve(Quantity);

		// Batch create entities with shared fragment values
		TSharedRef<FMassEntityManager::FEntityCreationContext> CreationContext =
			Manager->BatchCreateEntities(ArchetypeHandle, SharedFragmentValues, Quantity, SpawnedEntities);

		// Set initial fragment values if any were provided
		if (FragmentInstances.Num() > 0)
		{
			// Create entity collection for the spawned entities
			TArray<FMassArchetypeEntityCollection> EntityCollections;
			UE::Mass::Utils::CreateEntityCollections(
				*Manager,
				SpawnedEntities,
				FMassArchetypeEntityCollection::NoDuplicates,
				EntityCollections
			);

			// Apply initial fragment values to all spawned entities
			Manager->BatchSetEntityFragmentValues(EntityCollections, FragmentInstances);
		}

		return SpawnedEntities;
	}

	// Overload for spawning a single entity
	template<typename... TArgs>
	FORCEINLINE FMassEntityHandle BuildEntity(TArgs&&... Args) const
	{
		TArray<FMassEntityHandle> Entities = BuildEntities(1, Forward<TArgs>(Args)...);
		return Entities.Num() > 0 ? Entities[0] : FMassEntityHandle();
	}

	/**
	 * Spawns an entity deferentially using a raw command buffer.
	 * This is the base implementation that other overloads call.
	 * The entity is reserved immediately, and a command is pushed to build it with the specified fragments and tags.
	 *
	 * @param CommandBuffer The command buffer to push the build command to.
	 * @param Args A variadic list of fragment instances and/or tag types to build the entity with.
	 * @return An FMassEntityHandle for the entity that was reserved. The entity will not be active until the command buffer is flushed.
	 */
	template<typename... TArgs>
	FORCEINLINE FMassEntityHandle BuildEntityDefer(FMassCommandBuffer& CommandBuffer, TArgs&&... Args) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available"));

		const FMassEntityHandle ReservedEntity = Manager->ReserveEntity();

		CommandBuffer.PushCommand<FMassCommandBuildEntity>(ReservedEntity, Forward<TArgs>(Args)...); // [RESTORED] Original behavior.

		return ReservedEntity;
	}

	/**
	 * Spawns an entity deferentially using the execution context from a processor.
	 * The entity is reserved immediately, and a command is pushed to build it with the specified fragments and tags.
	 * (!!! MODIFIED: Automatically adds FEntityFlagFragment via the base overload !!!)
	 *
	 * @param Context The processor's execution context, used to get the command buffer.
	 * @param Args A variadic list of fragment instances and/or tag types to build the entity with.
	 * @return An FMassEntityHandle for the entity that was reserved. The entity will not be active until the command buffer is flushed.
	 */
	template<typename... TArgs>
	FORCEINLINE FMassEntityHandle BuildEntityDefer(FMassExecutionContext& Context, TArgs&&... Args) const
	{
		// This is a convenience wrapper that calls the base overload with the context's command buffer.
		// The base overload now handles adding the FEntityFlagFragment.
		return BuildEntityDefer(Context.Defer(), Forward<TArgs>(Args)...);
	}

	/**
	 * Defers the creation of a single entity using a template.
	 * The entity is reserved immediately, and a command is pushed to the command buffer to build the entity based on the template.
	 * @param Context The processor's execution context, used to get the command buffer.
	 * @param TemplateData The template defining the entity's archetype and initial values.
	 * @return A reserved FMassEntityHandle. The entity will not be active until the command buffer is flushed.
	 */
	FMassEntityHandle BuildEntityDefer(FMassExecutionContext& Context, FMassEntityTemplateData& TemplateData) const;

	FMassEntityHandle BuildEntityDefer(FMassCommandBuffer& CommandBuffer, FMassEntityTemplateData& TemplateData) const;

	/**
	 * Defers the creation of multiple entities using a template.
	 * The entities are reserved immediately, and a command is pushed to the command buffer to build them based on the template.
	 * @param Context The processor's execution context, used to get the command buffer.
	 * @param Quantity The number of entities to create.
	 * @param TemplateData The template defining the entities' archetype and initial values.
	 * @param OutEntities An array to be populated with the reserved entity handles.
	 */
	void BuildEntitiesDefer(FMassExecutionContext& Context, int32 Quantity, FMassEntityTemplateData& TemplateData, TArray<FMassEntityHandle>& OutEntities) const;

	void BuildEntitiesDefer(FMassCommandBuffer& CommandBuffer, int32 Quantity, FMassEntityTemplateData& TemplateData, TArray<FMassEntityHandle>& OutEntities) const;

	/**
	 * Destroys an entity immediately.
	 * @param EntityHandle The entity to destroy.
	 */
	FORCEINLINE void DestroyEntity(FMassEntityHandle EntityHandle) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available"));

		if (UNLIKELY(!Manager->IsEntityActive(EntityHandle))) return;
		Manager->DestroyEntity(EntityHandle);
	}

	/**
	 * Destroys an entity deferentially using the execution context from a processor.
	 * A command is pushed to destroy the entity, which will be processed when the command buffer is flushed.
	 * @param Context The processor's execution context, used to get the command buffer.
	 * @param EntityHandle The entity to destroy.
	 */
	FORCEINLINE void DestroyEntityDefer(FMassExecutionContext& Context, FMassEntityHandle EntityHandle) const
	{
		Context.Defer().DestroyEntity(EntityHandle);
	}

	/**
	 * Destroys an entity deferentially using a raw command buffer.
	 * A command is pushed to destroy the entity, which will be processed when the command buffer is flushed.
	 * @param CommandBuffer The command buffer to push the destroy command to.
	 * @param EntityHandle The entity to destroy.
	 */
	FORCEINLINE void DestroyEntityDefer(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle) const
	{
		CommandBuffer.DestroyEntity(EntityHandle);
	}

	//---------------- Template Data Operations -----------------

	/**
	 * @brief Gets a reference to a fragment within a specified FMassEntityTemplateData.
	 * @tparam TFragment The type of fragment to retrieve, which must inherit from FMassFragment.
	 * @param TemplateData The entity template data containing the fragment.
	 * @return A mutable reference to the fragment.
	 * @note This function will trigger an assertion (checkf) if the fragment type does not exist in the TemplateData,
	 * as it cannot return a valid reference. Ensure the fragment has been added via AddFragment or AddFragment_GetRef before calling.
	 */
	template<typename TFragment>
	FORCEINLINE TFragment& GetFragmentRef(FMassEntityTemplateData& TemplateData) const
	{
		TFragment* FragmentPtr = TemplateData.GetMutableFragment<TFragment>();
		checkf(FragmentPtr, TEXT("Attempted to get a reference to fragment '%s' which does not exist in the provided TemplateData. Please add the fragment first."), *TFragment::StaticStruct()->GetName());
		return *FragmentPtr;
	}

	//-------------- Entity Data Operations - Synchronous ---------------

	/**
	 * Check if entity has a specific fragment type using its UScriptStruct.
	 * This is the non-templated version for use when the type is a variable (e.g., from Blueprints).
	 * @param EntityHandle The entity to check.
	 * @param FragmentType The type of the fragment to check for. Can be null.
	 * @return True if the entity has the fragment, false otherwise.
	 */
	FORCEINLINE bool HasFragment(FMassEntityHandle EntityHandle, const UScriptStruct* FragmentType) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for HasFragment"));

		// Correct Implementation: Get the fragment struct view and check if it's valid.
		return Manager->GetFragmentDataStruct(EntityHandle, FragmentType).IsValid();
	}

	/**
	 * Check if entity has a specific tag type using its UScriptStruct.
	 * This is the non-templated version for use when the type is a variable (e.g., from Blueprints).
	 * @param EntityHandle The entity to check.
	 * @param TagType The type of the tag to check for. Can be null.
	 * @return True if the entity has the tag, false otherwise.
	 */
	FORCEINLINE bool HasTag(FMassEntityHandle EntityHandle, const UScriptStruct* TagType) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for HasTag"));

		// Correct Implementation: Construct an FMassEntityView and use its HasTag method.
		const FMassEntityView EntityView(*Manager, EntityHandle);
		return EntityView.HasTag(*TagType);
	}

	/**
	 * Check if entity has a specific fragment type
	 * @param EntityHandle The entity to check
	 * @return true if entity has the fragment
	 */
	template<typename T>
	FORCEINLINE bool HasFragment(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CFragment<T>,
			"T must be a valid fragment type inheriting from FMassFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for HasFragment"));

		return Manager->GetFragmentDataPtr<T>(EntityHandle) != nullptr;
	}

	/**
	 * Check if entity has a specific tag type
	 * @param EntityHandle The entity to check
	 * @return true if entity has the tag
	 */
	template<typename T>
	FORCEINLINE bool HasTag(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CTag<T>,
			"T must be a valid tag type inheriting from FMassTag");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for HasTag"));

		FMassEntityView EntityView(*Manager, EntityHandle);
		return EntityView.HasTag<T>();
	}

	/**
	 * Get fragment data by value (copy)
	 * @param EntityHandle The entity to get fragment from
	 * @return Copy of the fragment data
	 */
	template<typename T>
	FORCEINLINE T GetFragment(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CFragment<T>,
			"T must be a valid fragment type inheriting from FMassFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for GetFragment"));

		T* FragmentPtr = Manager->GetFragmentDataPtr<T>(EntityHandle);
		checkf(FragmentPtr, TEXT("Entity does not have fragment of type %s"), *T::StaticStruct()->GetName());

		return *FragmentPtr;
	}

	/**
	 * Get fragment data pointer for modification
	 * @param EntityHandle The entity to get fragment from
	 * @return Pointer to fragment data, nullptr if not found
	 */
	template<typename T>
	FORCEINLINE T* GetFragmentPtr(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CFragment<T>,
			"T must be a valid fragment type inheriting from FMassFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for GetFragment"));

		return Manager->GetFragmentDataPtr<T>(EntityHandle);
	}

	/**
	 * Get fragment data reference for modification
	 * @param EntityHandle The entity to get fragment from
	 * @return Reference to fragment data
	 */
	template<typename T>
	FORCEINLINE T& GetFragmentRef(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CFragment<T>,
			"T must be a valid fragment type inheriting from FMassFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for GetFragmentRef"));

		T* FragmentPtr = Manager->GetFragmentDataPtr<T>(EntityHandle);
		checkf(FragmentPtr, TEXT("Entity does not have fragment of type %s"), *T::StaticStruct()->GetName());

		return *FragmentPtr;
	}

	/**
	 * Add a fragment to an entity immediately.
	 * NOTE: If the entity already has this fragment, a warning will be logged and the operation will be ignored.
	 * The existing fragment's value will NOT be changed.
	 * @param EntityHandle The entity to add the fragment to
	 * @param FragmentValue The initial value for the new fragment
	 */
	template<typename T>
	FORCEINLINE void AddFragment(FMassEntityHandle EntityHandle, const T& FragmentValue) const
	{
		static_assert(UE::Mass::CFragment<T>,
			"T must be a valid fragment type inheriting from FMassFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for AddFragment"));
		FInstancedStruct FragmentInstance = FInstancedStruct::Make(FragmentValue);
		Manager->AddFragmentInstanceListToEntity(EntityHandle, MakeArrayView(&FragmentInstance, 1));
	}

	/**
	 * Add a fragment to an entity immediately using an FInstancedStruct.
	 * NOTE: If the entity already has this fragment, a warning will be logged and the operation will be ignored.
	 * The existing fragment's value will NOT be changed.
	 * @param EntityHandle The entity to add the fragment to
	 * @param FragmentStruct The instanced struct containing the fragment type and its initial value
	 */
	FORCEINLINE void AddFragment(FMassEntityHandle EntityHandle, const FInstancedStruct& FragmentStruct) const
	{
		checkf(FragmentStruct.IsValid(), TEXT("The provided FInstancedStruct is not valid."));
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for AddFragment"));
		Manager->AddFragmentInstanceListToEntity(EntityHandle, MakeArrayView(&FragmentStruct, 1));
	}

	/**
	 * Remove a fragment from the entity
	 * @param EntityHandle The entity to remove fragment from
	 * @return true if fragment was removed, false if entity didn't have the fragment
	 */
	template<typename T>
	FORCEINLINE void RemoveFragment(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CFragment<T>,
			"T must be a valid fragment type inheriting from FMassFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for RemoveFragment"));
		const UScriptStruct* FragmentType = T::StaticStruct();
		Manager->RemoveFragmentFromEntity(EntityHandle, FragmentType);
	}

	/**
	 * Remove a fragment from the entity
	 * (Non-templated version for BP use)
	 * @param EntityHandle The entity to remove fragment from
	 * @param FragmentType The fragment type to remove.
	 * @return True if fragment was removed, false if entity didn't have the fragment or was invalid.
	 */
	FORCEINLINE bool RemoveFragment(FMassEntityHandle EntityHandle, UScriptStruct* FragmentType)
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for RemoveFragment"));
		if (UNLIKELY(!FragmentType) || !FragmentType->IsChildOf(FMassFragment::StaticStruct())) return false;
		Manager->RemoveFragmentFromEntity(EntityHandle, FragmentType);  // This function takes pointer and returns void
		return true;  // Successfully called remove
	}

	/**
	 * Add a tag to the entity
	 * @param EntityHandle The entity to add tag to
	 */
	template<typename T>
	FORCEINLINE void AddTag(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CTag<T>,
			"T must be a valid tag type inheriting from FMassTag");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for AddTag"));
		const UScriptStruct* TagType = T::StaticStruct();
		Manager->AddTagToEntity(EntityHandle, TagType);
	}

	/**
	 * Remove a tag from the entity
	 * @param EntityHandle The entity to remove tag from
	 * @return true if tag was removed, false if entity didn't have the tag
	 */
	template<typename T>
	FORCEINLINE void RemoveTag(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CTag<T>,
			"T must be a valid tag type inheriting from FMassTag");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for RemoveTag"));
		const UScriptStruct* TagType = T::StaticStruct();
		Manager->RemoveTagFromEntity(EntityHandle, TagType);
	}

	/**
	 * Check if entity has a specific shared fragment type
	 * @param EntityHandle The entity to check
	 * @return true if entity has the shared fragment
	 */
	template<typename T>
	FORCEINLINE void HasSharedFragment(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CSharedFragment<T>,
			"T must be a valid shared fragment type inheriting from FMassSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for HasSharedFragment"));

		return Manager->GetSharedFragmentDataPtr<T>(EntityHandle) != nullptr;
	}

	/**
	 * Check if entity has a specific const shared fragment type
	 * @param EntityHandle The entity to check
	 * @return true if entity has the const shared fragment
	 */
	template<typename T>
	FORCEINLINE void HasConstSharedFragment(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CConstSharedFragment<T>,
			"T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for HasConstSharedFragment"));

		return Manager->GetConstSharedFragmentDataPtr<T>(EntityHandle) != nullptr;
	}

	/**
	 * Get shared fragment data by value (copy)
	 * @param EntityHandle The entity to get shared fragment from
	 * @return Copy of the shared fragment data
	 */
	template<typename T>
	FORCEINLINE T GetSharedFragment(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CSharedFragment<T>,
			"T must be a valid shared fragment type inheriting from FMassSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for GetSharedFragment"));

		T* SharedFragmentPtr = Manager->GetSharedFragmentDataPtr<T>(EntityHandle);
		checkf(SharedFragmentPtr, TEXT("Entity does not have shared fragment of type %s"), *T::StaticStruct()->GetName());

		return *SharedFragmentPtr;
	}

	/**
	 * Get const shared fragment data by value (copy)
	 * @param EntityHandle The entity to get const shared fragment from
	 * @return Copy of the const shared fragment data
	 */
	template<typename T>
	FORCEINLINE T GetConstSharedFragment(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CConstSharedFragment<T>,
			"T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for GetConstSharedFragment"));

		const T* ConstSharedFragmentPtr = Manager->GetConstSharedFragmentDataPtr<T>(EntityHandle);
		checkf(ConstSharedFragmentPtr, TEXT("Entity does not have const shared fragment of type %s"), *T::StaticStruct()->GetName());

		return *ConstSharedFragmentPtr;
	}

	/**
	 * Get shared fragment data pointer
	 * @param EntityHandle The entity to get shared fragment from
	 * @return Pointer to shared fragment data, nullptr if not found
	 */
	template<typename T>
	FORCEINLINE T* GetSharedFragmentPtr(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CSharedFragment<T>,
			"T must be a valid shared fragment type inheriting from FMassSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for GetSharedFragmentPtr"));

		return Manager->GetSharedFragmentDataPtr<T>(EntityHandle);
	}

	/**
	 * Get const shared fragment data pointer
	 * @param EntityHandle The entity to get const shared fragment from
	 * @return Pointer to const shared fragment data, nullptr if not found
	 */
	template<typename T>
	FORCEINLINE const T* GetConstSharedFragmentPtr(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CConstSharedFragment<T>,
			"T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for GetConstSharedFragmentPtr"));

		return Manager->GetConstSharedFragmentDataPtr<T>(EntityHandle);
	}

	/**
	 * Get shared fragment data reference
	 * @param EntityHandle The entity to get shared fragment from
	 * @return Reference to shared fragment data
	 */
	template<typename T>
	FORCEINLINE T& GetSharedFragmentRef(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CSharedFragment<T>,
			"T must be a valid shared fragment type inheriting from FMassSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for GetSharedFragmentRef"));

		T* SharedFragmentPtr = Manager->GetSharedFragmentDataPtr<T>(EntityHandle);
		checkf(SharedFragmentPtr, TEXT("Entity does not have shared fragment of type %s"), *T::StaticStruct()->GetName());

		return *SharedFragmentPtr;
	}

	/**
	 * Get const shared fragment data const reference
	 * @param EntityHandle The entity to get const shared fragment from
	 * @return Const reference to const shared fragment data
	 */
	template<typename T>
	FORCEINLINE const T& GetConstSharedFragmentRef(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CConstSharedFragment<T>,
			"T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");

		const FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for GetConstSharedFragmentRef"));

		const T* ConstSharedFragmentPtr = Manager->GetConstSharedFragmentDataPtr<T>(EntityHandle);
		checkf(ConstSharedFragmentPtr, TEXT("Entity does not have const shared fragment of type %s"), *T::StaticStruct()->GetName());

		return *ConstSharedFragmentPtr;
	}

	/**
	 * Add a shared fragment to an entity immediately
	 * @param EntityHandle The entity to add the shared fragment to
	 * @param SharedFragmentValue The shared fragment value
	 * @return true if successfully added, false if entity already has it with different value
	 */
	template<typename T>
	FORCEINLINE bool AddSharedFragment(FMassEntityHandle EntityHandle, const T& SharedFragmentValue) const
	{
		static_assert(UE::Mass::CSharedFragment<T>,
			"T must be a valid shared fragment type inheriting from FMassSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for GetSharedFragmentRef"));

		const FSharedStruct& SharedStruct = Manager->GetOrCreateSharedFragment(SharedFragmentValue);
		return Manager->AddSharedFragmentToEntity(EntityHandle, SharedStruct);
	}

	/**
	 * Add a const shared fragment to an entity immediately
	 * @param EntityHandle The entity to add the const shared fragment to
	 * @param ConstSharedFragmentValue The const shared fragment value
	 * @return true if successfully added, false if entity already has it with different value
	 */
	template<typename T>
	FORCEINLINE bool AddConstSharedFragment(FMassEntityHandle EntityHandle, const T& ConstSharedFragmentValue) const
	{
		static_assert(UE::Mass::CConstSharedFragment<T>,
			"T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for AddConstSharedFragment"));

		const FConstSharedStruct& ConstSharedStruct = Manager->GetOrCreateConstSharedFragment(ConstSharedFragmentValue);
		return Manager->AddConstSharedFragmentToEntity(EntityHandle, ConstSharedStruct);
	}

	/**
	 * Remove a shared fragment from the entity
	 * @param EntityHandle The entity to remove shared fragment from
	 * @return true if shared fragment was removed, false if entity didn't have it
	 */
	template<typename T>
	FORCEINLINE bool RemoveSharedFragment(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CSharedFragment<T>,
			"T must be a valid shared fragment type inheriting from FMassSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for RemoveSharedFragment"));

		return Manager->RemoveSharedFragmentFromEntity(EntityHandle, *T::StaticStruct());
	}

	/**
	 * Remove a shared fragment from the entity
	 * (Non-templated version for BP use)
	 * @param EntityHandle The entity to remove shared fragment from
	 * @param SharedFragmentType The shared fragment type to remove.
	 * @return true if shared fragment was removed, false if entity didn't have it or was invalid.
	 */
	FORCEINLINE bool RemoveSharedFragment(FMassEntityHandle EntityHandle, const UScriptStruct* SharedFragmentType) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for RemoveSharedFragment"));
		if (UNLIKELY(!SharedFragmentType) || !SharedFragmentType->IsChildOf(FMassSharedFragment::StaticStruct())) return false;
		return Manager->RemoveSharedFragmentFromEntity(EntityHandle, *SharedFragmentType);  // Dereference pointer!
	}

	/**
	 * Remove a const shared fragment from the entity
	 * @param EntityHandle The entity to remove const shared fragment from
	 * @return true if const shared fragment was removed, false if entity didn't have it
	 */
	template<typename T>
	FORCEINLINE bool RemoveConstSharedFragment(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CConstSharedFragment<T>,
			"T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for RemoveConstSharedFragment"));

		return Manager->RemoveConstSharedFragmentFromEntity(EntityHandle, *T::StaticStruct());
	}

	/**
	 * Remove a const shared fragment from the entity
	 * (Non-templated version for BP use)
	 * @param EntityHandle The entity to remove const shared fragment from
	 * @param ConstSharedFragmentType The const shared fragment type to remove.
	 * @return true if const shared fragment was removed, false if entity didn't have it or was invalid.
	 */
	FORCEINLINE bool RemoveConstSharedFragment(FMassEntityHandle EntityHandle, const UScriptStruct* ConstSharedFragmentType) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for RemoveConstSharedFragment"));
		if (UNLIKELY(!ConstSharedFragmentType) || !ConstSharedFragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct())) return false;
		return Manager->RemoveConstSharedFragmentFromEntity(EntityHandle, *ConstSharedFragmentType);  // Dereference pointer!
	}


	//--------------- (新) Flag Operations ---------------

	/**
	 * 获取实体当前的 64 位标志位掩码。
	 * @param EntityHandle The entity to check.
	 * @return The 64-bit flag mask. Returns 0 if the entity is invalid or has no flag fragment.
	 */
	int64 GetEntityFlags(FMassEntityHandle EntityHandle) const;

	/**
	 * 检查实体是否拥有某个特定标志。
	 * @param EntityHandle The entity to check.
	 * @param FlagToTest The flag to check for.
	 * @return True if the entity has the FEntityFlagFragment and the flag is set, false otherwise.
	 */
	bool HasEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags FlagToTest) const;

	/**
	 * 向实体添加一个标志。
	 * (如果 FEntityFlagFragment 不存在，会自动添加)
	 * @param EntityHandle The entity to modify.
	 * @param FlagToSet The flag to add.
	 * @return True if the flag was successfully set (or was already set), false on failure (e.g., invalid entity).
	 */
	bool SetEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags FlagToSet) const;

	/**
	 * 从实体移除一个标志。
	 * (如果 FEntityFlagFragment 不存在，此操作无效)
	 * @param EntityHandle The entity to modify.
	 * @param FlagToClear The flag to remove.
	 * @return True if the flag was successfully cleared (or was already clear), false on failure.
	 */
	bool ClearEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags FlagToClear) const;


	//--------------- Entity Data Operations - Deferred with Execution Context ---------------

	/**
	* Deferred add fragment with default value using execution context
	* Safe for use in Mass processors
	* @param Context The execution context from the processor
	* @param EntityHandle The entity to add the fragment to
	*/
	template<typename T>
	FORCEINLINE void AddFragment(FMassExecutionContext& Context, FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		Context.Defer().AddFragment<T>(EntityHandle);
	}

	/**
	 * Deferred add fragment with specific value using execution context
	 * @param Context The execution context from the processor
	 * @param EntityHandle The entity to add the fragment to
	 * @param FragmentValue The initial value for the new fragment
	 */
	template<typename T>
	FORCEINLINE void AddFragment(FMassExecutionContext& Context, FMassEntityHandle EntityHandle, const T& FragmentValue) const
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		Context.Defer().PushCommand<FMassCommandAddFragmentInstances>(EntityHandle, FragmentValue);
	}

	/**
	 * Deferred remove fragment using execution context
	 * @param Context The execution context from the processor
	 * @param EntityHandle The entity to remove fragment from
	 */
	template<typename T>
	FORCEINLINE void RemoveFragment(FMassExecutionContext& Context, FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		Context.Defer().RemoveFragment<T>(EntityHandle);
	}

	/**
	 * Deferred add tag using execution context
	 * Safe for use in Mass processors
	 * @param Context The execution context from the processor
	 * @param EntityHandle The entity to add tag to
	 */
	template<typename T>
	FORCEINLINE void AddTag(FMassExecutionContext& Context, FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CTag<T>, "T must be a valid tag type inheriting from FMassTag");
		Context.Defer().AddTag<T>(EntityHandle);
	}

	/**
	 * Deferred remove tag using execution context
	 * @param Context The execution context from the processor
	 * @param EntityHandle The entity to remove tag from
	 */
	template<typename T>
	FORCEINLINE void RemoveTag(FMassExecutionContext& Context, FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CTag<T>, "T must be a valid tag type inheriting from FMassTag");
		Context.Defer().RemoveTag<T>(EntityHandle);
	}

	/**
	 * Deferred swap tags using execution context
	 * @param Context The execution context from the processor
	 * @param EntityHandle The entity to swap tags on
	 */
	template<typename TOld, typename TNew>
	FORCEINLINE void SwapTags(FMassExecutionContext& Context, FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CTag<TOld>, "TOld must be a valid tag type inheriting from FMassTag");
		static_assert(UE::Mass::CTag<TNew>, "TNew must be a valid tag type inheriting from FMassTag");
		Context.Defer().SwapTags<TOld, TNew>(EntityHandle);
	}


	//--------------- Entity Data Operations - Deferred with Command Buffer ---------------

	/**
	 * Deferred add fragment with default value using command buffer
	 * Use this when you have direct access to a command buffer
	 * @param CommandBuffer The command buffer to use
	 * @param EntityHandle The entity to add the fragment to
	 */
	template<typename T>
	FORCEINLINE void AddFragment(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		CommandBuffer.AddFragment<T>(EntityHandle);
	}

	/**
	 * Deferred add fragment with specific value using command buffer
	 * @param CommandBuffer The command buffer to use
	 * @param EntityHandle The entity to add the fragment to
	 * @param FragmentValue The initial value for the new fragment
	 */
	template<typename T>
	FORCEINLINE void AddFragment(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle, const T& FragmentValue) const
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		CommandBuffer.PushCommand<FMassCommandAddFragmentInstances>(EntityHandle, FragmentValue);
	}

	/**
	 * Deferred remove fragment using command buffer
	 * @param CommandBuffer The command buffer to use
	 * @param EntityHandle The entity to remove fragment from
	 */
	template<typename T>
	FORCEINLINE void RemoveFragment(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		CommandBuffer.RemoveFragment<T>(EntityHandle);
	}

	/**
	 * Deferred add tag using command buffer
	 * Use this when you have direct access to a command buffer
	 * @param CommandBuffer The command buffer to use
	 * @param EntityHandle The entity to add tag to
	 */
	template<typename T>
	FORCEINLINE void AddTag(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CTag<T>, "T must be a valid tag type inheriting from FMassTag");
		CommandBuffer.AddTag<T>(EntityHandle);
	}

	/**
	 * Deferred remove tag using command buffer
	 * @param CommandBuffer The command buffer to use
	 * @param EntityHandle The entity to remove tag from
	 */
	template<typename T>
	FORCEINLINE void RemoveTag(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CTag<T>, "T must be a valid tag type inheriting from FMassTag");
		CommandBuffer.RemoveTag<T>(EntityHandle);
	}

	/**
	 * Deferred swap tags using command buffer
	 * @param CommandBuffer The command buffer to use
	 * @param EntityHandle The entity to swap tags on
	 */
	template<typename TOld, typename TNew>
	FORCEINLINE void SwapTags(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CTag<TOld>, "TOld must be a valid tag type inheriting from FMassTag");
		static_assert(UE::Mass::CTag<TNew>, "TNew must be a valid tag type inheriting from FMassTag");
		CommandBuffer.SwapTags<TOld, TNew>(EntityHandle);
	}


private:
	//------------------- Other ---------------

	UPROPERTY()
	mutable TObjectPtr<UWorld> CurrentWorld;

	UPROPERTY()
	mutable TObjectPtr<UMassEntitySubsystem> MassEntitySubsystem;

	mutable FMassEntityManager* EntityManager = nullptr;

protected:

	// Check if EntityHandle is valid, will trigger an assertion
	FORCEINLINE void CheckIfEntityIsActive(FMassEntityHandle EntityHandle) const
	{
		const FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available"));
		Manager->CheckIfEntityIsActive(EntityHandle);
	}
};
