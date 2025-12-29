/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#include "MassAPISubsystem.h"
#include "Engine/World.h"
#include "MassCommands.h"
#include "MassEntityManager.h"
#include "MassAPIFuncLib.h"
#include "MassEntityQuery.h"
#include "MassEntitySubsystem.h"

// Define a log category for MassAPI, or use LogTemp if you prefer.
DEFINE_LOG_CATEGORY_STATIC(LogMassAPI, Log, All);

void UMassAPISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UMassEntitySubsystem>();
    GetEntityManager();
    UE_LOG(LogTemp, Log, TEXT("MassAPISubsystem Initialized."));
}

void UMassAPISubsystem::Deinitialize()
{
    EntityManager = nullptr;
    MassEntitySubsystem = nullptr;
    CurrentWorld = nullptr;
    Super::Deinitialize();
    UE_LOG(LogTemp, Log, TEXT("MassAPISubsystem Deinitialized."));
}

TStatId UMassAPISubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UMassAPISubsystem, STATGROUP_Tickables);
}

UMassAPISubsystem* UMassAPISubsystem::GetPtr()
{
    if (!GWorld) return nullptr;
    return GWorld->GetSubsystem<UMassAPISubsystem>();
}

UMassAPISubsystem* UMassAPISubsystem::GetPtr(const UObject* WorldContextObject)
{
    if (!WorldContextObject) return nullptr;
    const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World) return nullptr;
    return World->GetSubsystem<UMassAPISubsystem>();
}

UMassAPISubsystem& UMassAPISubsystem::GetRef()
{
    UMassAPISubsystem* Subsystem = GetPtr();
    checkf(Subsystem, TEXT("Failed to get MassAPISubsystem from GWorld. Make sure GWorld is valid and the subsystem is initialized."));
    return *Subsystem;
}

UMassAPISubsystem& UMassAPISubsystem::GetRef(const UObject* WorldContextObject)
{
    UMassAPISubsystem* Subsystem = GetPtr(WorldContextObject);
    checkf(Subsystem, TEXT("Failed to get MassAPISubsystem from world context. Make sure the world is valid and the subsystem is initialized."));
    return *Subsystem;
}

FMassEntityManager* UMassAPISubsystem::GetEntityManager() const
{
    // If we've already cached the pointer, return it immediately.
    // This is the fast path that will be taken most of the time.
    if (EntityManager)
    {
        return EntityManager;
    }

    // --- Lazy-load path ---
    // Since this is a const function, all member variables we modify
    // from here on MUST be marked as 'mutable'.

    if (!CurrentWorld)
    {
        CurrentWorld = GetWorld();
    }

    if (CurrentWorld && !MassEntitySubsystem)
    {
        MassEntitySubsystem = CurrentWorld->GetSubsystem<UMassEntitySubsystem>();
    }

    if (MassEntitySubsystem)
    {
        // We use GetMutableEntityManager because the API requires it, but our function remains const.
        // This is safe because we are only caching a pointer, not changing the logical state of this subsystem.
        EntityManager = &MassEntitySubsystem->GetMutableEntityManager();
    }

    return EntityManager;
}

void UMassAPISubsystem::Tick(float DeltaTime)
{
    TRACE_CPUPROFILER_EVENT_SCOPE_STR("MassAPISubsystemTick");

    if (GetEntityManager())
    {
        EntityManager->FlushCommands();
    }
}

TArray<FMassEntityHandle> UMassAPISubsystem::BuildEntities(int32 Quantity, FMassEntityTemplateData& TemplateData) const
{
    TArray<FMassEntityHandle> SpawnedEntities;
    FMassEntityManager* Manager = GetEntityManager();
    checkf(Manager, TEXT("EntityManager is not available"));

    // Validate input
    if (Quantity <= 0)
    {
        return SpawnedEntities;
    }

    TemplateData.Sort();

    // Create an archetype from the MODIFIED template data composition
    FMassArchetypeHandle ArchetypeHandle = Manager->CreateArchetype
    (
        TemplateData.GetCompositionDescriptor(),
        TemplateData.GetArchetypeCreationParams()
    );

    // Check if archetype creation was successful
    if (!ArchetypeHandle.IsValid())
    {
        return SpawnedEntities;
    }

    // Get shared fragment values from the template
    const FMassArchetypeSharedFragmentValues& SharedFragmentValues = TemplateData.GetSharedFragmentValues();

    // Batch create entities with the archetype and shared fragments
    TSharedRef<FMassEntityManager::FEntityCreationContext> CreationContext =
        Manager->BatchCreateEntities(ArchetypeHandle, SharedFragmentValues, Quantity, SpawnedEntities);

    // Set initial fragment values if provided
    TConstArrayView<FInstancedStruct> InitialFragmentValues = TemplateData.GetInitialFragmentValues();

    if (InitialFragmentValues.Num() > 0)
    {
        // Create entity collection for the spawned entities
        TArray<FMassArchetypeEntityCollection> EntityCollections;

        UE::Mass::Utils::CreateEntityCollections
        (
            *Manager,
            SpawnedEntities,
            FMassArchetypeEntityCollection::NoDuplicates,
            EntityCollections
        );

        // Apply initial fragment values to all spawned entities
        Manager->BatchSetEntityFragmentValues(EntityCollections, InitialFragmentValues);
    }

    return SpawnedEntities;
}

FMassEntityHandle UMassAPISubsystem::BuildEntity(FMassEntityTemplateData& TemplateData) const
{
    auto Handles = BuildEntities(1, TemplateData);

    if (!Handles.IsEmpty())
    {
        return Handles[0];
    }
    else
    {
        return FMassEntityHandle();
    }
}

FMassEntityHandle UMassAPISubsystem::BuildEntityDefer(FMassExecutionContext& Context, FMassEntityTemplateData& TemplateData) const
{
    FMassEntityManager* Manager = GetEntityManager();
    checkf(Manager, TEXT("EntityManager is not available"));

    // 1. Reserve entity handle
    const FMassEntityHandle ReservedEntity = Manager->ReserveEntity();
    const FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor();
    const FMassArchetypeCreationParams CreationParams = TemplateData.GetArchetypeCreationParams();
    const FMassArchetypeSharedFragmentValues SharedValues = TemplateData.GetSharedFragmentValues();
    const TArray<FInstancedStruct> InitialFragments(TemplateData.GetInitialFragmentValues());

    // 3. Push deferred creation command
    Context.Defer().PushCommand<FMassDeferredCreateCommand>([ReservedEntity, Composition, CreationParams, SharedValues, InitialFragments](FMassEntityManager& Manager)
        {
            // Get or create archetype (using the modified Composition)
            const FMassArchetypeHandle ArchetypeHandle = Manager.CreateArchetype(Composition, CreationParams);
            if (!ArchetypeHandle.IsValid())
            {
                // If archetype creation fails, release the reserved entity to prevent leaks
                Manager.ReleaseReservedEntity(ReservedEntity);
                return;
            }

            // Build entity with archetype and shared fragments
            Manager.BuildEntity(ReservedEntity, ArchetypeHandle, SharedValues);

            // Set initial fragment values
            if (InitialFragments.Num() > 0)
            {
                Manager.SetEntityFragmentValues(ReservedEntity, InitialFragments);
            }
        });

    return ReservedEntity;
}

FMassEntityHandle UMassAPISubsystem::BuildEntityDefer(FMassCommandBuffer& CommandBuffer, FMassEntityTemplateData& TemplateData) const
{
    FMassEntityManager* Manager = GetEntityManager();
    checkf(Manager, TEXT("EntityManager is not available"));

    // 1. Reserve entity handle
    const FMassEntityHandle ReservedEntity = Manager->ReserveEntity();

    TemplateData.Sort();
    const FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor();
    const FMassArchetypeCreationParams CreationParams = TemplateData.GetArchetypeCreationParams();
    const FMassArchetypeSharedFragmentValues SharedValues = TemplateData.GetSharedFragmentValues();
    const TArray<FInstancedStruct> InitialFragments(TemplateData.GetInitialFragmentValues());

    // 3. Push deferred creation command
    CommandBuffer.PushCommand<FMassDeferredCreateCommand>([ReservedEntity, Composition, CreationParams, SharedValues, InitialFragments](FMassEntityManager& Manager)
        {
            // Get or create archetype (using the modified Composition)
            const FMassArchetypeHandle ArchetypeHandle = Manager.CreateArchetype(Composition, CreationParams);
            if (!ArchetypeHandle.IsValid())
            {
                // If archetype creation fails, release the reserved entity to prevent leaks
                Manager.ReleaseReservedEntity(ReservedEntity);
                return;
            }

            // Build entity with archetype and shared fragments
            Manager.BuildEntity(ReservedEntity, ArchetypeHandle, SharedValues);

            // Set initial fragment values
            if (InitialFragments.Num() > 0)
            {
                Manager.SetEntityFragmentValues(ReservedEntity, InitialFragments);
            }
        });

    return ReservedEntity;
}

void UMassAPISubsystem::BuildEntitiesDefer(FMassExecutionContext& Context, int32 Quantity, FMassEntityTemplateData& TemplateData, TArray<FMassEntityHandle>& OutEntities) const
{
    FMassEntityManager* Manager = GetEntityManager();
    checkf(Manager, TEXT("EntityManager is not available"));

    // 1. Batch reserve entities
    TArray<FMassEntityHandle> ReservedEntities;
    ReservedEntities.AddUninitialized(Quantity);
    Manager->BatchReserveEntities(MakeArrayView(ReservedEntities));

    // Add reserved handles to the output array
    OutEntities.Append(ReservedEntities);

    const FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor();
    const FMassArchetypeCreationParams CreationParams = TemplateData.GetArchetypeCreationParams();
    const FMassArchetypeSharedFragmentValues SharedValues = TemplateData.GetSharedFragmentValues();
    const TArray<FInstancedStruct> InitialFragments(TemplateData.GetInitialFragmentValues());

    // 3. Push deferred creation command
    Context.Defer().PushCommand<FMassDeferredCreateCommand>([ReservedEntities, Composition, CreationParams, SharedValues, InitialFragments](FMassEntityManager& Manager)
        {
            // Get or create archetype (using the modified Composition)
            const FMassArchetypeHandle ArchetypeHandle = Manager.CreateArchetype(Composition, CreationParams);
            if (!ArchetypeHandle.IsValid())
            {
                // If archetype creation fails, release all reserved entities
                for (const FMassEntityHandle& Entity : ReservedEntities)
                {
                    Manager.ReleaseReservedEntity(Entity);
                }
                return;
            }

            // Batch build reserved entities
            Manager.BatchCreateReservedEntities(ArchetypeHandle, SharedValues, ReservedEntities);

            // Batch set initial fragment values
            if (InitialFragments.Num() > 0)
            {
                TArray<FMassArchetypeEntityCollection> EntityCollections;
                UE::Mass::Utils::CreateEntityCollections(Manager, ReservedEntities, FMassArchetypeEntityCollection::NoDuplicates, EntityCollections);
                Manager.BatchSetEntityFragmentValues(EntityCollections, InitialFragments);
            }
        });
}

void UMassAPISubsystem::BuildEntitiesDefer(FMassCommandBuffer& CommandBuffer, int32 Quantity, FMassEntityTemplateData& TemplateData, TArray<FMassEntityHandle>& OutEntities) const
{
    FMassEntityManager* Manager = GetEntityManager();
    checkf(Manager, TEXT("EntityManager is not available"));

    // 1. Batch reserve entities
    TArray<FMassEntityHandle> ReservedEntities;
    ReservedEntities.AddUninitialized(Quantity);
    Manager->BatchReserveEntities(MakeArrayView(ReservedEntities));

    // Add reserved handles to the output array
    OutEntities.Append(ReservedEntities);

    const FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor();
    const FMassArchetypeCreationParams CreationParams = TemplateData.GetArchetypeCreationParams();
    const FMassArchetypeSharedFragmentValues SharedValues = TemplateData.GetSharedFragmentValues();
    const TArray<FInstancedStruct> InitialFragments(TemplateData.GetInitialFragmentValues());

    // 3. Push deferred creation command
    CommandBuffer.PushCommand<FMassDeferredCreateCommand>([ReservedEntities, Composition, CreationParams, SharedValues, InitialFragments](FMassEntityManager& Manager)
        {
            // Get or create archetype (using the modified Composition)
            const FMassArchetypeHandle ArchetypeHandle = Manager.CreateArchetype(Composition, CreationParams);
            if (!ArchetypeHandle.IsValid())
            {
                // If archetype creation fails, release all reserved entities
                for (const FMassEntityHandle& Entity : ReservedEntities)
                {
                    Manager.ReleaseReservedEntity(Entity);
                }
                return;
            }

            // Batch build reserved entities
            Manager.BatchCreateReservedEntities(ArchetypeHandle, SharedValues, ReservedEntities);

            // Batch set initial fragment values
            if (InitialFragments.Num() > 0)
            {
                TArray<FMassArchetypeEntityCollection> EntityCollections;
                UE::Mass::Utils::CreateEntityCollections(Manager, ReservedEntities, FMassArchetypeEntityCollection::NoDuplicates, EntityCollections);
                Manager.BatchSetEntityFragmentValues(EntityCollections, InitialFragments);
            }
        });
}

//----------------------------------------------------------------------//
// (新) Flag Fragment Operations
//----------------------------------------------------------------------//

int64 UMassAPISubsystem::GetEntityFlags(FMassEntityHandle EntityHandle) const
{
    FMassEntityManager* Manager = GetEntityManager();
    if (!Manager || !Manager->IsEntityValid(EntityHandle))
    {
        return 0;
    }

    if (const FEntityFlagFragment* FlagFragment = Manager->GetFragmentDataPtr<FEntityFlagFragment>(EntityHandle))
    {
        return FlagFragment->Flags;
    }

    // Log a warning if the fragment is missing
    UE_LOG(LogMassAPI, Warning, TEXT("GetEntityFlags: Entity does not have FEntityFlagFragment. Add the fragment to the entity template or manually."));
    return 0;
}

bool UMassAPISubsystem::HasEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags FlagToTest) const
{
    // Validation handled inside fragment method, but we check validity here first
    if (FlagToTest >= EEntityFlags::EEntityFlags_MAX)
    {
        return false;
    }

    FMassEntityManager* Manager = GetEntityManager();
    if (!Manager || !Manager->IsEntityValid(EntityHandle))
    {
        return false;
    }

    if (const FEntityFlagFragment* FlagFragment = Manager->GetFragmentDataPtr<FEntityFlagFragment>(EntityHandle))
    {
        return FlagFragment->HasFlag(FlagToTest);
    }

    // No fragment = flag not set
    return false;
}

bool UMassAPISubsystem::SetEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags FlagToSet) const
{
    if (FlagToSet >= EEntityFlags::EEntityFlags_MAX)
    {
        return false;
    }

    FMassEntityManager* Manager = GetEntityManager();
    if (!Manager || !Manager->IsEntityValid(EntityHandle))
    {
        return false;
    }

    // Get mutable pointer
    if (FEntityFlagFragment* FlagFragment = Manager->GetFragmentDataPtr<FEntityFlagFragment>(EntityHandle))
    {
        FlagFragment->SetFlag(FlagToSet);
        return true;
    }

    UE_LOG(LogMassAPI, Warning, TEXT("SetEntityFlag: Entity does not have FEntityFlagFragment. Flag not set."));
    return false;
}

bool UMassAPISubsystem::ClearEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags FlagToClear) const
{
    if (FlagToClear >= EEntityFlags::EEntityFlags_MAX)
    {
        return false;
    }

    FMassEntityManager* Manager = GetEntityManager();
    if (!Manager || !Manager->IsEntityValid(EntityHandle))
    {
        return false;
    }

    // Get mutable pointer
    if (FEntityFlagFragment* FlagFragment = Manager->GetFragmentDataPtr<FEntityFlagFragment>(EntityHandle))
    {
        FlagFragment->ClearFlag(FlagToClear);
        return true;
    }

    UE_LOG(LogMassAPI, Warning, TEXT("ClearEntityFlag: Entity does not have FEntityFlagFragment. Flag not cleared."));
    return false;
}

//----------------------------------------------------------------------//
// Entity Query Iteration
//----------------------------------------------------------------------//

void UMassAPISubsystem::ExecuteForEach(const UObject* WorldContextObject, const FEntityQuery& Query)
{
    // 获取所有匹配的实体
    TArray<FEntityHandle> MatchingEntities = UMassAPIFuncLib::GetMatchingEntities(WorldContextObject, Query);

    // 遍历每个实体并触发委托
    for (int32 Index = 0; Index < MatchingEntities.Num(); ++Index)
    {
        const FEntityHandle& Entity = MatchingEntities[Index];

        // 触发委托 - 这会调用蓝图中的 LoopBody 执行引脚
        OnEntityIterate.Broadcast(Entity, Index);
    }
}

