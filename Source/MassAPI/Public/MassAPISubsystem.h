/*
* MassAPI
* Created: 2025
* Author: Leroy Works, Ember, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "MassSubsystemBase.h"
#include "MassAPIStructs.h"
#include "MassAPIEnums.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "MassEntityView.h"
#include "MassEntityTemplate.h"
#include "MassCommands.h"
#include "MassCommandBuffer.h"
#include "MassExecutionContext.h"
#include "Stats/StatsSystemTypes.h"
#include "Subsystems/SubsystemCollection.h"
#include "MassAPIVersion.h"

#include "MassAPISubsystem.generated.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// 声明每次迭代的委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEntityIterate, FEntityHandle, Element, int32, Index);


//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/**
 * UMassAPISubsystem provides convenient API extensions for MassEntity framework
 * This subsystem wraps common entity operations for easier access
 */
UCLASS()
class MASSAPI_API UMassAPISubsystem : public UMassTickableSubsystemBase
{
	GENERATED_BODY()


protected:

	// Subsystem lifecycle
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;

	// This function is required by UTickableWorldSubsystem for profiling
	virtual TStatId GetStatId() const override;

public:

	static UMassAPISubsystem* GetPtr();
	static UMassAPISubsystem* GetPtr(const UObject* WorldContextObject);

	static UMassAPISubsystem& GetRef();
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

	// A contains all of B | A 包含 B 的全部
	FORCEINLINE static bool HasAll(const FMassArchetypeCompositionDescriptor& ThisComposition, const FMassArchetypeCompositionDescriptor& OtherComposition)
	{
		return HAS_ALL_COMPOSITION(ThisComposition, OtherComposition);
	}

	// A contains any of B | A 包含 B 的任一
	FORCEINLINE static bool HasAny(const FMassArchetypeCompositionDescriptor& ThisComposition, const FMassArchetypeCompositionDescriptor& OtherComposition)
	{
		return HAS_ANY_COMPOSITION(ThisComposition, OtherComposition);
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

		// 从查询中获取缓存的位掩码 (低位 0-63)
		const int64 AllFlagsQueryLow = Query.GetAllFlagsBitmask();
		const int64 AnyFlagsQueryLow = Query.GetAnyFlagsBitmask();
		const int64 NoneFlagsQueryLow = Query.GetNoneFlagsBitmask();

		// 从查询中获取缓存的位掩码 (高位 64-127)
		const int64 AllFlagsQueryHigh = Query.GetAllFlagsBitmaskHigh();
		const int64 AnyFlagsQueryHigh = Query.GetAnyFlagsBitmaskHigh();
		const int64 NoneFlagsQueryHigh = Query.GetNoneFlagsBitmaskHigh();

		// 只有在有标志位查询时才执行检查
		if (AllFlagsQueryLow == 0 && AnyFlagsQueryLow == 0 && NoneFlagsQueryLow == 0 &&
		    AllFlagsQueryHigh == 0 && AnyFlagsQueryHigh == 0 && NoneFlagsQueryHigh == 0)
		{
			return true; // 没有标志位查询，总是通过
		}

		// 尝试获取实体的 Flag Fragment
		const FEntityFlagFragment* FlagFragment = Manager->GetFragmentDataPtr<FEntityFlagFragment>(EntityHandle);

		// 如果实体没有 Flag Fragment，则认为它的标志位为 0
		const int64 EntityFlagsLow = FlagFragment ? FlagFragment->Flags : 0;
		const int64 EntityFlagsHigh = FlagFragment ? FlagFragment->FlagsHigh : 0;

		// 执行标志位检查 (低位 + 高位)
		// All flags: must have ALL in both low and high
		const bool bAllLow = (AllFlagsQueryLow == 0) || ((EntityFlagsLow & AllFlagsQueryLow) == AllFlagsQueryLow);
		const bool bAllHigh = (AllFlagsQueryHigh == 0) || ((EntityFlagsHigh & AllFlagsQueryHigh) == AllFlagsQueryHigh);
		const bool bAllFlags = bAllLow && bAllHigh;

		// Any flags: need at least one match across both
		const bool bAnyPass = (AnyFlagsQueryLow == 0 && AnyFlagsQueryHigh == 0)
		                    || ((EntityFlagsLow & AnyFlagsQueryLow) != 0)
		                    || ((EntityFlagsHigh & AnyFlagsQueryHigh) != 0);

		// None flags: must NOT have any in either
		const bool bNoneLow = (NoneFlagsQueryLow == 0) || ((EntityFlagsLow & NoneFlagsQueryLow) == 0);
		const bool bNoneHigh = (NoneFlagsQueryHigh == 0) || ((EntityFlagsHigh & NoneFlagsQueryHigh) == 0);
		const bool bNoneFlags = bNoneLow && bNoneHigh;

		return (bAllFlags && bAnyPass && bNoneFlags);
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
	 * Pull a const-shared fragment value back out of a FMassEntityTemplateData.
	 * Useful when caching values from a template at spawn-time without going through
	 * the per-agent fragment slot (e.g. for fragments that are registered as
	 * const-shared and intentionally have no per-agent counterpart).
	 * @return Pointer to the stored value, or nullptr if the template carries no
	 *         const-shared fragment of type T.
	 */
	template<typename T>
	static const T* FindConstSharedFragmentInTemplate(const FMassEntityTemplateData& Tmpl)
	{
		const auto& Composition = Tmpl.GetSharedFragmentValues();
		for (const FConstSharedStruct& SS : Composition.GetConstSharedFragments())
		{
			if (SS.GetScriptStruct() == T::StaticStruct())
			{
				// FConstSharedStruct::GetPtr<U>() is constrained to const U via TEnableIf.
				return SS.GetPtr<const T>();
			}
		}
		return nullptr;
	}

	/**
	 * Pull a (mutable) shared fragment value back out of a FMassEntityTemplateData.
	 * Mirror of FindConstSharedFragmentInTemplate for fragments registered via
	 * AddSharedFragment / GetOrCreateSharedFragment.
	 */
	template<typename T>
	static T* FindSharedFragmentInTemplate(const FMassEntityTemplateData& Tmpl)
	{
		const auto& Composition = Tmpl.GetSharedFragmentValues();
		for (const FSharedStruct& SS : Composition.GetSharedFragments())
		{
			if (SS.GetScriptStruct() == T::StaticStruct())
			{
				return SS.GetPtr<T>();
			}
		}
		return nullptr;
	}

	/**
	 * Replace (or add) a const-shared fragment of type T inside a FMassEntityTemplateData.
	 * Rebuilds the template from scratch, copying every tag / fragment / shared
	 * fragment except any pre-existing const-shared T, then dedupes the new value
	 * through the entity manager's const-shared store. Useful when a per-spawn
	 * tweak (e.g. ScaleMult baked into RenderShared.Transform) needs to land on
	 * a SharedTemplate copy that the rest of the spawn pipeline still reads from.
	 */
	template<typename T>
	static void ReplaceConstSharedFragmentInTemplate(
		FMassEntityTemplateData& Template,
		const T& NewValue,
		FMassEntityManager& EntityManager)
	{
		FMassEntityTemplateData NewData;

		// Carry over template name for debug parity.
		NewData.SetTemplateName(Template.GetTemplateName());

		// Tags
		Template.GetTags().ExportTypes([&NewData](const UScriptStruct* TagType)
		{
			TEMPLATE_ADD_TAG(NewData, TagType);
			return true;
		});

		// Fragments — composition + initial values
		Template.GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&NewData](const UScriptStruct* Type)
		{
			TEMPLATE_ADD_FRAGMENT(NewData, Type);
			return true;
		});
		for (const FInstancedStruct& Frag : Template.GetInitialFragmentValues())
		{
			NewData.AddFragment(FConstStructView(Frag));
		}

		// Chunk fragments are type-only in templates (no per-template payload to preserve);
		// the composition would still be reflected if any consumer relied on them, but the
		// MassBattle agent template doesn't register chunk fragments today.

		// Shared fragments — pass through
		for (const FSharedStruct& S : Template.GetSharedFragmentValues().GetSharedFragments())
		{
			NewData.AddSharedFragment(S);
		}

		// Const-shared fragments — skip any existing entry of type T, then add the new one
		for (const FConstSharedStruct& C : Template.GetSharedFragmentValues().GetConstSharedFragments())
		{
			if (C.GetScriptStruct() != T::StaticStruct())
			{
				NewData.AddConstSharedFragment(C);
			}
		}
		NewData.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment<T>(NewValue));

		Template = MoveTemp(NewData);
	}

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
						BIT_SET_ADD(Tags, ArgType::StaticStruct());
					}
					// Check if it's a regular Fragment
					else if constexpr (UE::Mass::CFragment<ArgType>)
					{
						BIT_SET_ADD(Fragments, ArgType::StaticStruct());
						FragmentInstances.Add(FInstancedStruct::Make(Forward<decltype(Arg)>(Arg)));
					}
					// Check if it's specifically a SharedFragment or ConstSharedFragment
					else if constexpr (UE::Mass::CSharedFragment<ArgType>)
					{
						BIT_SET_ADD(SharedFragments, ArgType::StaticStruct());
						// Create or get the shared fragment instance
						const FSharedStruct& SharedStruct = Manager->GetOrCreateSharedFragment(Forward<decltype(Arg)>(Arg));
						SharedFragmentValues.Add(SharedStruct);
					}
					else if constexpr (UE::Mass::CConstSharedFragment<ArgType>)
					{
						BIT_SET_ADD(ConstSharedFragments, ArgType::StaticStruct());
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

	// Build an entity using a template
	FMassEntityHandle BuildEntity(FMassEntityTemplateData& TemplateData) const;

	// Overload for spawning a single entity
	template<typename... TArgs>
	FORCEINLINE FMassEntityHandle BuildEntity(TArgs&&... Args) const
	{
		TArray<FMassEntityHandle> Entities = BuildEntities(1, Forward<TArgs>(Args)...);
		return Entities.Num() > 0 ? Entities[0] : FMassEntityHandle();
	}

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


	//-------------- Entity Data Operations ---------------

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

		// Use archetype composition check for performance and consistency
		const FMassArchetypeHandle Archetype = Manager->GetArchetypeForEntity(EntityHandle);
		if (!Archetype.IsValid())
		{
			return false;
		}
		const FMassArchetypeCompositionDescriptor& Composition = Manager->GetArchetypeComposition(Archetype);
		return CONTAINS_TAG(Composition, TagType);
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

		// Use archetype composition check for speed
		const FMassArchetypeHandle Archetype = Manager->GetArchetypeForEntity(EntityHandle);
		if (!Archetype.IsValid())
		{
			return false;
		}
		const FMassArchetypeCompositionDescriptor& Composition = Manager->GetArchetypeComposition(Archetype);
		return CONTAINS_T_TAG(Composition, T);
	}

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

		// Use archetype composition check for performance and consistency
		const FMassArchetypeHandle Archetype = Manager->GetArchetypeForEntity(EntityHandle);
		if (!Archetype.IsValid())
		{
			return false;
		}
		const FMassArchetypeCompositionDescriptor& Composition = Manager->GetArchetypeComposition(Archetype);
		return CONTAINS_FRAGMENT(Composition, FragmentType);
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

		// Use archetype composition check for speed
		const FMassArchetypeHandle Archetype = Manager->GetArchetypeForEntity(EntityHandle);
		if (!Archetype.IsValid())
		{
			return false;
		}
		const FMassArchetypeCompositionDescriptor& Composition = Manager->GetArchetypeComposition(Archetype);
		return CONTAINS_T_FRAGMENT(Composition, T);
	}

	/**
	 * Check if entity has a specific chunk fragment type using its UScriptStruct.
	 * @param EntityHandle The entity to check.
	 * @param ChunkFragmentType The type of the chunk fragment to check for.
	 * @return True if the entity has the chunk fragment, false otherwise.
	 */
	FORCEINLINE bool HasChunkFragment(FMassEntityHandle EntityHandle, const UScriptStruct* ChunkFragmentType) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for HasChunkFragment"));

		const FMassArchetypeHandle Archetype = Manager->GetArchetypeForEntity(EntityHandle);
		if (!Archetype.IsValid())
		{
			return false;
		}
		const FMassArchetypeCompositionDescriptor& Composition = Manager->GetArchetypeComposition(Archetype);
		return CONTAINS_CHUNK(Composition, ChunkFragmentType);
	}

	/**
	 * Check if entity has a specific chunk fragment type
	 * @param EntityHandle The entity to check
	 * @return true if entity has the chunk fragment
	 */
	template<typename T>
	FORCEINLINE bool HasChunkFragment(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CChunkFragment<T>,
			"T must be a valid chunk fragment type inheriting from FMassChunkFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for HasChunkFragment"));

		// Use archetype composition check for speed
		const FMassArchetypeHandle Archetype = Manager->GetArchetypeForEntity(EntityHandle);
		if (!Archetype.IsValid())
		{
			return false;
		}
		const FMassArchetypeCompositionDescriptor& Composition = Manager->GetArchetypeComposition(Archetype);
		return CONTAINS_T_CHUNK(Composition, T);
	}

	/**
	 * Check if entity has a specific shared fragment type using its UScriptStruct.
	 * @param EntityHandle The entity to check.
	 * @param SharedFragmentType The type of the shared fragment to check for.
	 * @return True if the entity has the shared fragment, false otherwise.
	 */
	FORCEINLINE bool HasSharedFragment(FMassEntityHandle EntityHandle, const UScriptStruct* SharedFragmentType) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for HasSharedFragment"));

		const FMassArchetypeHandle Archetype = Manager->GetArchetypeForEntity(EntityHandle);
		if (!Archetype.IsValid())
		{
			return false;
		}
		const FMassArchetypeCompositionDescriptor& Composition = Manager->GetArchetypeComposition(Archetype);
		return CONTAINS_SHARED(Composition, SharedFragmentType);
	}

	/**
	 * Check if entity has a specific shared fragment type
	 * @param EntityHandle The entity to check
	 * @return true if entity has the shared fragment
	 */
	template<typename T>
	FORCEINLINE bool HasSharedFragment(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CSharedFragment<T>,
			"T must be a valid shared fragment type inheriting from FMassSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for HasSharedFragment"));

		// Use archetype composition check for speed
		const FMassArchetypeHandle Archetype = Manager->GetArchetypeForEntity(EntityHandle);
		if (!Archetype.IsValid())
		{
			return false;
		}
		const FMassArchetypeCompositionDescriptor& Composition = Manager->GetArchetypeComposition(Archetype);
		return CONTAINS_T_SHARED(Composition, T);
	}

	/**
	 * Check if entity has a specific const shared fragment type using its UScriptStruct.
	 * @param EntityHandle The entity to check.
	 * @param ConstSharedFragmentType The type of the const shared fragment to check for.
	 * @return True if the entity has the const shared fragment, false otherwise.
	 */
	FORCEINLINE bool HasConstSharedFragment(FMassEntityHandle EntityHandle, const UScriptStruct* ConstSharedFragmentType) const
	{
		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for HasConstSharedFragment"));

		const FMassArchetypeHandle Archetype = Manager->GetArchetypeForEntity(EntityHandle);
		if (!Archetype.IsValid())
		{
			return false;
		}
		const FMassArchetypeCompositionDescriptor& Composition = Manager->GetArchetypeComposition(Archetype);
		return CONTAINS_CONST_SHARED(Composition, ConstSharedFragmentType);
	}

	/**
	 * Check if entity has a specific const shared fragment type
	 * @param EntityHandle The entity to check
	 * @return true if entity has the const shared fragment
	 */
	template<typename T>
	FORCEINLINE bool HasConstSharedFragment(FMassEntityHandle EntityHandle) const
	{
		static_assert(UE::Mass::CConstSharedFragment<T>,
			"T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");

		FMassEntityManager* Manager = GetEntityManager();
		checkf(Manager, TEXT("EntityManager is not available for HasConstSharedFragment"));

		// Use archetype composition check for speed
		const FMassArchetypeHandle Archetype = Manager->GetArchetypeForEntity(EntityHandle);
		if (!Archetype.IsValid())
		{
			return false;
		}
		const FMassArchetypeCompositionDescriptor& Composition = Manager->GetArchetypeComposition(Archetype);
		return CONTAINS_T_CONST_SHARED(Composition, T);
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

		return ENTITY_MANAGER_REMOVE_SHARED(Manager, EntityHandle, T::StaticStruct());
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
		return ENTITY_MANAGER_REMOVE_SHARED(Manager, EntityHandle, SharedFragmentType);
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

		return ENTITY_MANAGER_REMOVE_CONST_SHARED(Manager, EntityHandle, T::StaticStruct());
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
		return ENTITY_MANAGER_REMOVE_CONST_SHARED(Manager, EntityHandle, ConstSharedFragmentType);
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

	//---------------- Template Data Operations -----------------

	//=== Has — existence checks | 存在性检查 ============================================

	/** Check if template data contains a specific tag type | 检查模板数据是否包含特定标签类型 */
	template<typename T>
	FORCEINLINE static bool HasTag(const FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CTag<T>, "T must be a valid tag type inheriting from FMassTag");
		bool bFound = false;
		Tmpl.GetTags().ExportTypes([&bFound](const UScriptStruct* Type) {
			if (Type == T::StaticStruct()) { bFound = true; return false; }
			return true;
		});
		return bFound;
	}

	/** Check if template data contains a specific fragment type | 检查模板数据是否包含特定片段类型 */
	template<typename T>
	FORCEINLINE static bool HasFragment(const FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		return CONTAINS_T_FRAGMENT(Tmpl.GetCompositionDescriptor(), T);
	}

	/** Check if template data contains a specific chunk fragment type | 检查模板数据是否包含特定块片段类型 */
	template<typename T>
	FORCEINLINE static bool HasChunkFragment(const FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CChunkFragment<T>, "T must be a valid chunk fragment type inheriting from FMassChunkFragment");
		return CONTAINS_T_CHUNK(Tmpl.GetCompositionDescriptor(), T);
	}

	/** Check if template data contains a specific shared fragment type | 检查模板数据是否包含特定共享片段类型 */
	template<typename T>
	FORCEINLINE static bool HasSharedFragment(const FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CSharedFragment<T>, "T must be a valid shared fragment type inheriting from FMassSharedFragment");
		return CONTAINS_T_SHARED(Tmpl.GetCompositionDescriptor(), T);
	}

	/** Check if template data contains a specific const shared fragment type | 检查模板数据是否包含特定常量共享片段类型 */
	template<typename T>
	FORCEINLINE static bool HasConstSharedFragment(const FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CConstSharedFragment<T>, "T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");
		return CONTAINS_T_CONST_SHARED(Tmpl.GetCompositionDescriptor(), T);
	}

	//=== Get — value retrieval | 值获取 ==================================================

	/** Get fragment value by copy from template data | 从模板数据获取片段值（拷贝） */
	template<typename T>
	FORCEINLINE static T GetFragment(const FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		const TConstArrayView<FInstancedStruct> Values = Tmpl.GetInitialFragmentValues();
		for (const FInstancedStruct& Frag : Values)
		{
			if (Frag.GetScriptStruct() == T::StaticStruct())
			{
				return Frag.Get<T>();
			}
		}
		checkf(false, TEXT("Template does not contain fragment '%s'. Use HasFragment before GetFragment, or use GetFragmentPtr for nullable access."), *T::StaticStruct()->GetName());
		return T();
	}

	/** Get mutable pointer to fragment in template data, nullptr if not found | 获取模板数据中片段的可变指针，未找到则返回 nullptr */
	template<typename T>
	FORCEINLINE static T* GetFragmentPtr(FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		return Tmpl.GetMutableFragment<T>();
	}

	/** Get mutable reference to fragment in template data (asserts if missing) | 获取模板数据中片段的可变引用（缺失时断言） */
	template<typename T>
	FORCEINLINE static T& GetFragmentRef(FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		T* Ptr = Tmpl.GetMutableFragment<T>();
		checkf(Ptr, TEXT("Template does not contain fragment '%s'. Add it first via AddFragment or SetFragment."), *T::StaticStruct()->GetName());
		return *Ptr;
	}

	/** Get shared fragment value by copy from template data | 从模板数据获取共享片段值（拷贝） */
	template<typename T>
	FORCEINLINE static T GetSharedFragment(const FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CSharedFragment<T>, "T must be a valid shared fragment type inheriting from FMassSharedFragment");
		const T* Ptr = FindSharedFragmentInTemplate<T>(Tmpl);
		checkf(Ptr, TEXT("Template does not contain shared fragment '%s'."), *T::StaticStruct()->GetName());
		return *Ptr;
	}

	/** Get mutable pointer to shared fragment in template data, nullptr if not found | 获取模板数据中共享片段的可变指针，未找到则返回 nullptr */
	template<typename T>
	FORCEINLINE static T* GetSharedFragmentPtr(FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CSharedFragment<T>, "T must be a valid shared fragment type inheriting from FMassSharedFragment");
		// engine stores shared fragments as FSharedStruct internally, GetPtr<T>() returns T* | 引擎内部将共享片段存储为 FSharedStruct，GetPtr<T>() 返回 T*
		return FindSharedFragmentInTemplate<T>(Tmpl);
	}

	/** Get const shared fragment value by copy from template data | 从模板数据获取常量共享片段值（拷贝） */
	template<typename T>
	FORCEINLINE static T GetConstSharedFragment(const FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CConstSharedFragment<T>, "T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");
		const T* Ptr = FindConstSharedFragmentInTemplate<T>(Tmpl);
		checkf(Ptr, TEXT("Template does not contain const shared fragment '%s'."), *T::StaticStruct()->GetName());
		return *Ptr;
	}

	/** Get const pointer to const shared fragment in template data, nullptr if not found | 获取模板数据中常量共享片段的常量指针，未找到则返回 nullptr */
	template<typename T>
	FORCEINLINE static const T* GetConstSharedFragmentPtr(const FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CConstSharedFragment<T>, "T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");
		return FindConstSharedFragmentInTemplate<T>(Tmpl);
	}

	//=== Set — add or overwrite | 添加或覆盖 =============================================

	/** Set fragment value in template data (overwrites if exists, adds if not) | 在模板数据中设置片段值（存在则覆盖，不存在则添加） */
	template<typename T>
	FORCEINLINE static void SetFragment(FMassEntityTemplateData& Tmpl, const T& Value)
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		T* Ptr = Tmpl.GetMutableFragment<T>();
		if (Ptr)
		{
			*Ptr = Value;
		}
		else
		{
			Tmpl.AddFragment(FConstStructView::Make(Value));
		}
	}

	/** Set shared fragment value in template data (removes old if exists, then adds new) | 在模板数据中设置共享片段值（移除旧的然后添加新的） */
	template<typename T>
	FORCEINLINE static void SetSharedFragment(FMassEntityTemplateData& Tmpl, const T& Value, FMassEntityManager& Mgr)
	{
		static_assert(UE::Mass::CSharedFragment<T>, "T must be a valid shared fragment type inheriting from FMassSharedFragment");
		// copy-on-write: rebuild template, skip old T, add new value | 写时复制：重建模板，跳过旧 T，添加新值
		FMassEntityTemplateData NewData;
		NewData.SetTemplateName(Tmpl.GetTemplateName());
		Tmpl.GetTags().ExportTypes([&NewData](const UScriptStruct* Type) { TEMPLATE_ADD_TAG(NewData, Type); return true; });
		Tmpl.GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&NewData](const UScriptStruct* Type) { TEMPLATE_ADD_FRAGMENT(NewData, Type); return true; });
		for (const FInstancedStruct& Frag : Tmpl.GetInitialFragmentValues()) { NewData.AddFragment(FConstStructView(Frag)); }
		for (const FSharedStruct& S : Tmpl.GetSharedFragmentValues().GetSharedFragments())
		{
			if (S.GetScriptStruct() != T::StaticStruct()) NewData.AddSharedFragment(S);
		}
		for (const FConstSharedStruct& C : Tmpl.GetSharedFragmentValues().GetConstSharedFragments()) { NewData.AddConstSharedFragment(C); }
		NewData.AddSharedFragment(Mgr.GetOrCreateSharedFragment(Value));
		Tmpl = MoveTemp(NewData);
	}

	/** Set const shared fragment value in template data (removes old if exists, then adds new) | 在模板数据中设置常量共享片段值（移除旧的然后添加新的） */
	template<typename T>
	FORCEINLINE static void SetConstSharedFragment(FMassEntityTemplateData& Tmpl, const T& Value, FMassEntityManager& Mgr)
	{
		static_assert(UE::Mass::CConstSharedFragment<T>, "T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");
		FMassEntityTemplateData NewData;
		NewData.SetTemplateName(Tmpl.GetTemplateName());
		Tmpl.GetTags().ExportTypes([&NewData](const UScriptStruct* Type) { TEMPLATE_ADD_TAG(NewData, Type); return true; });
		Tmpl.GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&NewData](const UScriptStruct* Type) { TEMPLATE_ADD_FRAGMENT(NewData, Type); return true; });
		for (const FInstancedStruct& Frag : Tmpl.GetInitialFragmentValues()) { NewData.AddFragment(FConstStructView(Frag)); }
		for (const FSharedStruct& S : Tmpl.GetSharedFragmentValues().GetSharedFragments()) { NewData.AddSharedFragment(S); }
		for (const FConstSharedStruct& C : Tmpl.GetSharedFragmentValues().GetConstSharedFragments())
		{
			if (C.GetScriptStruct() != T::StaticStruct()) NewData.AddConstSharedFragment(C);
		}
		NewData.AddConstSharedFragment(Mgr.GetOrCreateConstSharedFragment(Value));
		Tmpl = MoveTemp(NewData);
	}

	//=== Add — append new (no-op if already exists) | 追加新的（已存在则无操作）=============

	/** Add a tag to template data | 向模板数据添加标签 */
	template<typename T>
	FORCEINLINE static void AddTag(FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CTag<T>, "T must be a valid tag type inheriting from FMassTag");
		TEMPLATE_ADD_TAG(Tmpl, T::StaticStruct());
	}

	/** Add a fragment with default value to template data | 向模板数据添加具有默认值的片段 */
	template<typename T>
	FORCEINLINE static void AddFragment(FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		TEMPLATE_ADD_FRAGMENT(Tmpl, T::StaticStruct());
	}

	/** Add a fragment with initial value to template data | 向模板数据添加具有初始值的片段 */
	template<typename T>
	FORCEINLINE static void AddFragment(FMassEntityTemplateData& Tmpl, const T& Value)
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		TEMPLATE_ADD_FRAGMENT(Tmpl, T::StaticStruct());
		Tmpl.AddFragment(FConstStructView::Make(Value));
	}

	/** Add a shared fragment to template data | 向模板数据添加共享片段 */
	template<typename T>
	FORCEINLINE static void AddSharedFragment(FMassEntityTemplateData& Tmpl, const T& Value, FMassEntityManager& Mgr)
	{
		static_assert(UE::Mass::CSharedFragment<T>, "T must be a valid shared fragment type inheriting from FMassSharedFragment");
		Tmpl.AddSharedFragment(Mgr.GetOrCreateSharedFragment(Value));
	}

	/** Add a const shared fragment to template data | 向模板数据添加常量共享片段 */
	template<typename T>
	FORCEINLINE static void AddConstSharedFragment(FMassEntityTemplateData& Tmpl, const T& Value, FMassEntityManager& Mgr)
	{
		static_assert(UE::Mass::CConstSharedFragment<T>, "T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");
		Tmpl.AddConstSharedFragment(Mgr.GetOrCreateConstSharedFragment(Value));
	}

	//=== Remove — copy-on-write exclude | 写时复制排除 ====================================

	/** Remove a tag from template data | 从模板数据移除标签 */
	template<typename T>
	FORCEINLINE static void RemoveTag(FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CTag<T>, "T must be a valid tag type inheriting from FMassTag");
		FMassEntityTemplateData NewData;
		NewData.SetTemplateName(Tmpl.GetTemplateName());
		Tmpl.GetTags().ExportTypes([&NewData](const UScriptStruct* Type) {
			if (Type != T::StaticStruct()) TEMPLATE_ADD_TAG(NewData, Type);
			return true;
		});
		Tmpl.GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&NewData](const UScriptStruct* Type) { TEMPLATE_ADD_FRAGMENT(NewData, Type); return true; });
		for (const FInstancedStruct& Frag : Tmpl.GetInitialFragmentValues()) { NewData.AddFragment(FConstStructView(Frag)); }
		for (const FSharedStruct& S : Tmpl.GetSharedFragmentValues().GetSharedFragments()) { NewData.AddSharedFragment(S); }
		for (const FConstSharedStruct& C : Tmpl.GetSharedFragmentValues().GetConstSharedFragments()) { NewData.AddConstSharedFragment(C); }
		Tmpl = MoveTemp(NewData);
	}

	/** Remove a fragment from template data | 从模板数据移除片段 */
	template<typename T>
	FORCEINLINE static void RemoveFragment(FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CFragment<T>, "T must be a valid fragment type inheriting from FMassFragment");
		FMassEntityTemplateData NewData;
		NewData.SetTemplateName(Tmpl.GetTemplateName());
		Tmpl.GetTags().ExportTypes([&NewData](const UScriptStruct* Type) { TEMPLATE_ADD_TAG(NewData, Type); return true; });
		Tmpl.GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&NewData](const UScriptStruct* Type) {
			if (Type != T::StaticStruct()) TEMPLATE_ADD_FRAGMENT(NewData, Type);
			return true;
		});
		for (const FInstancedStruct& Frag : Tmpl.GetInitialFragmentValues())
		{
			if (Frag.GetScriptStruct() != T::StaticStruct()) NewData.AddFragment(FConstStructView(Frag));
		}
		for (const FSharedStruct& S : Tmpl.GetSharedFragmentValues().GetSharedFragments()) { NewData.AddSharedFragment(S); }
		for (const FConstSharedStruct& C : Tmpl.GetSharedFragmentValues().GetConstSharedFragments()) { NewData.AddConstSharedFragment(C); }
		Tmpl = MoveTemp(NewData);
	}

	/** Remove a shared fragment from template data | 从模板数据移除共享片段 */
	template<typename T>
	FORCEINLINE static void RemoveSharedFragment(FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CSharedFragment<T>, "T must be a valid shared fragment type inheriting from FMassSharedFragment");
		FMassEntityTemplateData NewData;
		NewData.SetTemplateName(Tmpl.GetTemplateName());
		Tmpl.GetTags().ExportTypes([&NewData](const UScriptStruct* Type) { TEMPLATE_ADD_TAG(NewData, Type); return true; });
		Tmpl.GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&NewData](const UScriptStruct* Type) { TEMPLATE_ADD_FRAGMENT(NewData, Type); return true; });
		for (const FInstancedStruct& Frag : Tmpl.GetInitialFragmentValues()) { NewData.AddFragment(FConstStructView(Frag)); }
		for (const FSharedStruct& S : Tmpl.GetSharedFragmentValues().GetSharedFragments())
		{
			if (S.GetScriptStruct() != T::StaticStruct()) NewData.AddSharedFragment(S);
		}
		for (const FConstSharedStruct& C : Tmpl.GetSharedFragmentValues().GetConstSharedFragments()) { NewData.AddConstSharedFragment(C); }
		Tmpl = MoveTemp(NewData);
	}

	/** Remove a const shared fragment from template data | 从模板数据移除常量共享片段 */
	template<typename T>
	FORCEINLINE static void RemoveConstSharedFragment(FMassEntityTemplateData& Tmpl)
	{
		static_assert(UE::Mass::CConstSharedFragment<T>, "T must be a valid const shared fragment type inheriting from FMassConstSharedFragment");
		FMassEntityTemplateData NewData;
		NewData.SetTemplateName(Tmpl.GetTemplateName());
		Tmpl.GetTags().ExportTypes([&NewData](const UScriptStruct* Type) { TEMPLATE_ADD_TAG(NewData, Type); return true; });
		Tmpl.GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&NewData](const UScriptStruct* Type) { TEMPLATE_ADD_FRAGMENT(NewData, Type); return true; });
		for (const FInstancedStruct& Frag : Tmpl.GetInitialFragmentValues()) { NewData.AddFragment(FConstStructView(Frag)); }
		for (const FSharedStruct& S : Tmpl.GetSharedFragmentValues().GetSharedFragments()) { NewData.AddSharedFragment(S); }
		for (const FConstSharedStruct& C : Tmpl.GetSharedFragmentValues().GetConstSharedFragments())
		{
			if (C.GetScriptStruct() != T::StaticStruct()) NewData.AddConstSharedFragment(C);
		}
		Tmpl = MoveTemp(NewData);
	}

	//=== Merge — variadic composition merge | 可变参数组合合并 =============================

	/** Merge additional tags/fragments/shared fragments into template data | 将额外的标签/片段/共享片段合并到模板数据中 */
	template<typename... TArgs>
	FORCEINLINE static void MergeTemplate(FMassEntityTemplateData& Tmpl, TArgs&&... Args)
	{
		auto ProcessArg = [&](auto&& Arg)
		{
			using ArgType = std::decay_t<decltype(Arg)>;
			if constexpr (UE::Mass::CTag<ArgType>)
			{
				AddTag<ArgType>(Tmpl);
			}
			else if constexpr (UE::Mass::CFragment<ArgType>)
			{
				AddFragment<ArgType>(Tmpl, Forward<decltype(Arg)>(Arg));
			}
			else
			{
				static_assert(UE::Mass::TAlwaysFalse<ArgType>, "MergeTemplate args must be Tags or Fragments. Shared fragments require EntityManager — use AddSharedFragment directly.");
			}
		};
		(ProcessArg(Forward<TArgs>(Args)), ...);
	}

	//=== Clone — deep copy | 深拷贝 ======================================================

	/** Create a deep copy of template data | 创建模板数据的深拷贝 */
	FORCEINLINE static FMassEntityTemplateData CloneTemplate(const FMassEntityTemplateData& Source)
	{
		FMassEntityTemplateData NewData;
		NewData.SetTemplateName(Source.GetTemplateName());
		Source.GetTags().ExportTypes([&NewData](const UScriptStruct* Type) { TEMPLATE_ADD_TAG(NewData, Type); return true; });
		Source.GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&NewData](const UScriptStruct* Type) { TEMPLATE_ADD_FRAGMENT(NewData, Type); return true; });
		for (const FInstancedStruct& Frag : Source.GetInitialFragmentValues()) { NewData.AddFragment(FConstStructView(Frag)); }
		for (const FSharedStruct& S : Source.GetSharedFragmentValues().GetSharedFragments()) { NewData.AddSharedFragment(S); }
		for (const FConstSharedStruct& C : Source.GetSharedFragmentValues().GetConstSharedFragments()) { NewData.AddConstSharedFragment(C); }
		return NewData;
	}

	//--------------- Flag Operations ---------------

	/**
	 * 获取实体当前的低位 64 位标志位掩码 (flags 0-63)。
	 * @param EntityHandle The entity to check.
	 * @return The 64-bit flag mask. Returns 0 if the entity is invalid or has no flag fragment.
	 */
	int64 GetEntityFlags(FMassEntityHandle EntityHandle) const;

	/**
	 * 获取实体当前的高位 64 位标志位掩码 (flags 64-127)。
	 * @param EntityHandle The entity to check.
	 * @return The 64-bit high flag mask. Returns 0 if the entity is invalid or has no flag fragment.
	 */
	int64 GetEntityFlagsHigh(FMassEntityHandle EntityHandle) const;

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

	// Deferred set flag — CmdBuf overload | 延迟设置标志 — CmdBuf 重载
	FORCEINLINE void SetEntityFlagDefer(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle, EEntityFlags FlagToSet) const
	{
		if (FlagToSet >= EEntityFlags::EEntityFlags_MAX) return;
			CommandBuffer.PushCommand<FMassDeferredSetCommand>([EntityHandle, FlagToSet](FMassEntityManager& Manager)
			{
				if (Manager.IsEntityValid(EntityHandle))
				{
					if (FEntityFlagFragment* Frag = Manager.GetFragmentDataPtr<FEntityFlagFragment>(EntityHandle))
					{
						Frag->SetFlag(FlagToSet);
					}
				}
			});
	}

	// Deferred set flag — Context overload | 延迟设置标志 — Context 重载
	FORCEINLINE void SetEntityFlagDefer(FMassExecutionContext& Context, FMassEntityHandle EntityHandle, EEntityFlags FlagToSet) const
	{
		SetEntityFlagDefer(Context.Defer(), EntityHandle, FlagToSet);
	}

	// Deferred clear flag — CmdBuf overload | 延迟清除标志 — CmdBuf 重载
	FORCEINLINE void ClearEntityFlagDefer(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle, EEntityFlags FlagToClear) const
	{
		if (FlagToClear >= EEntityFlags::EEntityFlags_MAX) return;
			CommandBuffer.PushCommand<FMassDeferredSetCommand>([EntityHandle, FlagToClear](FMassEntityManager& Manager)
			{
				if (Manager.IsEntityValid(EntityHandle))
				{
					if (FEntityFlagFragment* Frag = Manager.GetFragmentDataPtr<FEntityFlagFragment>(EntityHandle))
					{
						Frag->ClearFlag(FlagToClear);
					}
				}
			});
	}

	// Deferred clear flag — Context overload | 延迟清除标志 — Context 重载
	FORCEINLINE void ClearEntityFlagDefer(FMassExecutionContext& Context, FMassEntityHandle EntityHandle, EEntityFlags FlagToClear) const
	{
		ClearEntityFlagDefer(Context.Defer(), EntityHandle, FlagToClear);
	}


	//--------------- Entity Query Iteration ---------------

	/**
	 * 每次迭代时触发的委托
	 * 用于 ForEachMatchingEntities 蓝图节点
	 */
	UPROPERTY(BlueprintAssignable, Category = "MassAPI")
	FOnEntityIterate OnEntityIterate;

	/**
	 * 执行遍历匹配的实体，对每个实体触发委托
	 * 这是一个同步的遍历过程，使用委托机制来实现蓝图节点的多执行引脚输出
	 * @param WorldContextObject 世界上下文对象
	 * @param Query 查询条件
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Query", meta = (WorldContext = "WorldContextObject"))
	void ExecuteForEach(const UObject* WorldContextObject, UPARAM(ref) const FEntityQuery& Query);


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
