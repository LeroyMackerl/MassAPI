/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#include "MassAPISubsystem.h"
#include "Engine/World.h"
#include "MassCommands.h"
#include "MassEntityManager.h" // [ADDED] Include for logging entity handle

// [ADDED] Define a log category for MassAPI, or use LogTemp if you prefer.
DEFINE_LOG_CATEGORY_STATIC(LogMassAPI, Log, All);

void UMassAPISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // It's good practice to pre-cache the pointers here if possible,
    // but the lazy-loader will handle it if not.
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

UMassAPISubsystem* UMassAPISubsystem::GetPtr(const UObject* WorldContextObject)
{
    if (!WorldContextObject) return nullptr;
    const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World) return nullptr;
    return World->GetSubsystem<UMassAPISubsystem>();
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

    // (!!! MODIFICATION: Create a copy of the composition and add FEntityFlagFragment !!!)
    // FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor(); // [DELETED]
    // Composition.Fragments.Add(*FEntityFlagFragment::StaticStruct()); // [DELETED]

    // Create an archetype from the MODIFIED template data composition
    FMassArchetypeHandle ArchetypeHandle = Manager->CreateArchetype
    (
        TemplateData.GetCompositionDescriptor(), // [RESTORED] Use original composition
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

    // The CreationContext will automatically notify observers when it goes out of scope

    return SpawnedEntities;
}

FMassEntityHandle UMassAPISubsystem::BuildEntityDefer(FMassExecutionContext& Context, FMassEntityTemplateData& TemplateData) const
{
    FMassEntityManager* Manager = GetEntityManager();
    checkf(Manager, TEXT("EntityManager is not available"));

    // 1. Reserve entity handle
    const FMassEntityHandle ReservedEntity = Manager->ReserveEntity();

    // 2. Capture template data by value for deferred execution
    // (!!! MODIFICATION: Create a copy of the composition and add FEntityFlagFragment !!!)
    // FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor(); // [DELETED]
    // Composition.Fragments.Add(*FEntityFlagFragment::StaticStruct()); // [DELETED]

    const FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor(); // [RESTORED]
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

    // 2. Capture template data by value for deferred execution
    // (!!! MODIFICATION: Create a copy of the composition and add FEntityFlagFragment !!!)
    // FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor(); // [DELETED]
    // Composition.Fragments.Add(*FEntityFlagFragment::StaticStruct()); // [DELETED]

    const FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor(); // [RESTORED]
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

    // 2. Capture template data by value
    // (!!! MODIFICATION: Create a copy of the composition and add FEntityFlagFragment !!!)
    // FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor(); // [DELETED]
    // Composition.Fragments.Add(*FEntityFlagFragment::StaticStruct()); // [DELETED]

    const FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor(); // [RESTORED]
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

    // 2. Capture template data by value
    // (!!! MODIFICATION: Create a copy of the composition and add FEntityFlagFragment !!!)
    // FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor(); // [DELETED]
    // Composition.Fragments.Add(*FEntityFlagFragment::StaticStruct()); // [DELETED]

    const FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor(); // [RESTORED]
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

    // [MODIFIED] Log a warning if the fragment is missing
    UE_LOG(LogMassAPI, Warning, TEXT("GetEntityFlags: Entity does not have FEntityFlagFragment. Add the fragment to the entity template or manually."));
    return 0;
}

bool UMassAPISubsystem::HasEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags FlagToTest) const
{
    if (FlagToTest >= EEntityFlags::EEntityFlags_MAX)
    {
        return false;
    }

    // [MODIFIED] Get fragment directly without logging.
    FMassEntityManager* Manager = GetEntityManager();
    if (!Manager || !Manager->IsEntityValid(EntityHandle))
    {
        return false;
    }

    if (const FEntityFlagFragment* FlagFragment = Manager->GetFragmentDataPtr<FEntityFlagFragment>(EntityHandle))
    {
        // [MODIFIED] Perform check directly
        return (FlagFragment->Flags & (1LL << static_cast<uint8>(FlagToTest))) != 0;
    }

    // const int64 EntityFlags = GetEntityFlags(EntityHandle); // [DELETED] This would log a warning
    // return (EntityFlags & (1LL << static_cast<uint8>(FlagToTest))) != 0; // [DELETED]

    // No fragment, so it can't have the flag. No log.
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

    // 如果实体没有该 Fragment，则为其添加
    // Use the subsystem's own HasFragment method
    // if (!HasFragment<FEntityFlagFragment>(EntityHandle)) // [DELETED]
    // { // [DELETED]
    // 	Manager->AddFragmentToEntity(EntityHandle, FEntityFlagFragment::StaticStruct()); // [DELETED]
    // } // [DELETED]

    // 获取可变指针
    if (FEntityFlagFragment* FlagFragment = Manager->GetFragmentDataPtr<FEntityFlagFragment>(EntityHandle))
    {
        FlagFragment->Flags |= (1LL << static_cast<uint8>(FlagToSet));
        return true;
    }

    // [MODIFIED] Log a warning if the fragment is missing
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

    // 如果实体没有该 Fragment，则无需执行任何操作
    if (FEntityFlagFragment* FlagFragment = Manager->GetFragmentDataPtr<FEntityFlagFragment>(EntityHandle))
    {
        FlagFragment->Flags &= ~(1LL << static_cast<uint8>(FlagToClear));
        return true;
    }

    // [MODIFIED] Log a warning if the fragment is missing
    UE_LOG(LogMassAPI, Warning, TEXT("ClearEntityFlag: Entity does not have FEntityFlagFragment. Flag not cleared."));
    return false;
}

