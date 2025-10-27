// MassAPISubsystem.cpp
#include "MassAPISubsystem.h"
#include "Engine/World.h"
#include "MassCommands.h"


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

    // Reserve space for the entities array
    SpawnedEntities.Reserve(Quantity);

    // Create an archetype from the template data
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
    const FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor();
    const FMassArchetypeCreationParams CreationParams = TemplateData.GetArchetypeCreationParams();
    const FMassArchetypeSharedFragmentValues SharedValues = TemplateData.GetSharedFragmentValues();
    const TArray<FInstancedStruct> InitialFragments(TemplateData.GetInitialFragmentValues());

    // 3. Push deferred creation command
    Context.Defer().PushCommand<FMassDeferredCreateCommand>([ReservedEntity, Composition, CreationParams, SharedValues, InitialFragments](FMassEntityManager& EntityManager)
        {
            // Get or create archetype
            const FMassArchetypeHandle ArchetypeHandle = EntityManager.CreateArchetype(Composition, CreationParams);
            if (!ArchetypeHandle.IsValid())
            {
                // If archetype creation fails, release the reserved entity to prevent leaks
                EntityManager.ReleaseReservedEntity(ReservedEntity);
                return;
            }

            // Build entity with archetype and shared fragments
            EntityManager.BuildEntity(ReservedEntity, ArchetypeHandle, SharedValues);

            // Set initial fragment values
            if (InitialFragments.Num() > 0)
            {
                EntityManager.SetEntityFragmentValues(ReservedEntity, InitialFragments);
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
    const FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor();
    const FMassArchetypeCreationParams CreationParams = TemplateData.GetArchetypeCreationParams();
    const FMassArchetypeSharedFragmentValues SharedValues = TemplateData.GetSharedFragmentValues();
    const TArray<FInstancedStruct> InitialFragments(TemplateData.GetInitialFragmentValues());

    // 3. Push deferred creation command
    CommandBuffer.PushCommand<FMassDeferredCreateCommand>([ReservedEntity, Composition, CreationParams, SharedValues, InitialFragments](FMassEntityManager& EntityManager)
        {
            // Get or create archetype
            const FMassArchetypeHandle ArchetypeHandle = EntityManager.CreateArchetype(Composition, CreationParams);
            if (!ArchetypeHandle.IsValid())
            {
                // If archetype creation fails, release the reserved entity to prevent leaks
                EntityManager.ReleaseReservedEntity(ReservedEntity);
                return;
            }

            // Build entity with archetype and shared fragments
            EntityManager.BuildEntity(ReservedEntity, ArchetypeHandle, SharedValues);

            // Set initial fragment values
            if (InitialFragments.Num() > 0)
            {
                EntityManager.SetEntityFragmentValues(ReservedEntity, InitialFragments);
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
    const FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor();
    const FMassArchetypeCreationParams CreationParams = TemplateData.GetArchetypeCreationParams();
    const FMassArchetypeSharedFragmentValues SharedValues = TemplateData.GetSharedFragmentValues();
    const TArray<FInstancedStruct> InitialFragments(TemplateData.GetInitialFragmentValues());

    // 3. Push deferred creation command
    Context.Defer().PushCommand<FMassDeferredCreateCommand>([ReservedEntities, Composition, CreationParams, SharedValues, InitialFragments](FMassEntityManager& EntityManager)
        {
            // Get or create archetype
            const FMassArchetypeHandle ArchetypeHandle = EntityManager.CreateArchetype(Composition, CreationParams);
            if (!ArchetypeHandle.IsValid())
            {
                // If archetype creation fails, release all reserved entities
                for (const FMassEntityHandle& Entity : ReservedEntities)
                {
                    EntityManager.ReleaseReservedEntity(Entity);
                }
                return;
            }

            // Batch build reserved entities
            EntityManager.BatchCreateReservedEntities(ArchetypeHandle, SharedValues, ReservedEntities);

            // Batch set initial fragment values
            if (InitialFragments.Num() > 0)
            {
                TArray<FMassArchetypeEntityCollection> EntityCollections;
                UE::Mass::Utils::CreateEntityCollections(EntityManager, ReservedEntities, FMassArchetypeEntityCollection::NoDuplicates, EntityCollections);
                EntityManager.BatchSetEntityFragmentValues(EntityCollections, InitialFragments);
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
    const FMassArchetypeCompositionDescriptor Composition = TemplateData.GetCompositionDescriptor();
    const FMassArchetypeCreationParams CreationParams = TemplateData.GetArchetypeCreationParams();
    const FMassArchetypeSharedFragmentValues SharedValues = TemplateData.GetSharedFragmentValues();
    const TArray<FInstancedStruct> InitialFragments(TemplateData.GetInitialFragmentValues());

    // 3. Push deferred creation command
    CommandBuffer.PushCommand<FMassDeferredCreateCommand>([ReservedEntities, Composition, CreationParams, SharedValues, InitialFragments](FMassEntityManager& EntityManager)
        {
            // Get or create archetype
            const FMassArchetypeHandle ArchetypeHandle = EntityManager.CreateArchetype(Composition, CreationParams);
            if (!ArchetypeHandle.IsValid())
            {
                // If archetype creation fails, release all reserved entities
                for (const FMassEntityHandle& Entity : ReservedEntities)
                {
                    EntityManager.ReleaseReservedEntity(Entity);
                }
                return;
            }

            // Batch build reserved entities
            EntityManager.BatchCreateReservedEntities(ArchetypeHandle, SharedValues, ReservedEntities);

            // Batch set initial fragment values
            if (InitialFragments.Num() > 0)
            {
                TArray<FMassArchetypeEntityCollection> EntityCollections;
                UE::Mass::Utils::CreateEntityCollections(EntityManager, ReservedEntities, FMassArchetypeEntityCollection::NoDuplicates, EntityCollections);
                EntityManager.BatchSetEntityFragmentValues(EntityCollections, InitialFragments);
            }
        });
}

