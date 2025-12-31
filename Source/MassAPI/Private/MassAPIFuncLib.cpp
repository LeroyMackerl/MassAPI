/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#include "MassAPIFuncLib.h"
#include "MassAPISubsystem.h"
#include "MassAPIEnums.h"
#include "MassEntityView.h"
#include "MassEntitySubsystem.h"
#include "UObject/StructOnScope.h"
#include "MassEntityQuery.h"
#include "MassCommands.h"
#include "MassCommandBuffer.h"
#include "MassRequirements.h"
#include "MassExternalSubsystemTraits.h"
#include "MassDebugger.h"
#include "HAL/Platform.h"
#include "Logging/LogMacros.h"
#include "MassAPIStructs.h"
#include "Runtime/Launch/Resources/Version.h" 

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
// UE 5.7 accessor methods
#define GET_TAGS GetTags()
#define GET_FRAGMENTS GetFragments()
#define GET_CHUNK_FRAGMENTS GetChunkFragments()
#define GET_SHARED_FRAGMENTS GetSharedFragments()
#define GET_CONST_SHARED_FRAGMENTS GetConstSharedFragments()
#else
// < UE 5.7 accessor methods
#define GET_TAGS Tags
#define GET_FRAGMENTS Fragments
#define GET_CHUNK_FRAGMENTS ChunkFragments
#define GET_SHARED_FRAGMENTS SharedFragments
#define GET_CONST_SHARED_FRAGMENTS ConstSharedFragments
#endif

// Define a category to make filtering logs easier
DEFINE_LOG_CATEGORY_STATIC(LogMassBlueprintAPI, Log, All);


void UMassAPIFuncLib::FlushMassCommands(const UObject* WorldContextObject)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        if (FMassEntityManager* Manager = MassAPI->GetEntityManager())
        {
            Manager->FlushCommands();
        }
    }
}

//================ Entity Operations											    							========

bool UMassAPIFuncLib::IsValid(const UObject* WorldContextObject, const FEntityHandle& EntityHandle)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        return MassAPI->IsValid(EntityHandle);
    }
    return false;
}

//———————— Destroy.Entity																					    	————

void UMassAPIFuncLib::DestroyEntity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, const bool bDeferred, const FOnMassDeferredFinished OnFinished)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        if (bDeferred)
        {
            // Use a lambda to destroy and then callback
            // Note: FMassDeferredDestroyCommand implies the action of destroying.
            // Since we need to attach a callback, we use a generic command lambda.
            MassAPI->Defer().PushCommand<FMassDeferredDestroyCommand>([EntityHandle, OnFinished](FMassEntityManager& Manager)
                {
                    if (Manager.IsEntityValid(EntityHandle))
                    {
                        Manager.DestroyEntity(EntityHandle);
                        OnFinished.ExecuteIfBound(EntityHandle);
                    }
                });
        }
        else
        {
            MassAPI->DestroyEntity(EntityHandle);
            OnFinished.ExecuteIfBound(EntityHandle);
        }
    }
}

void UMassAPIFuncLib::DestroyEntities(const UObject* WorldContextObject, const TArray<FEntityHandle>& EntityHandles, const bool bDeferred, const FOnMassDeferredFinished OnFinished)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || EntityHandles.Num() == 0) return;

    TArray<FMassEntityHandle> MassHandles;
    MassHandles.Reserve(EntityHandles.Num());

    for (const FEntityHandle& Handle : EntityHandles)
    {
        MassHandles.Add(Handle);
    }

    if (bDeferred)
    {
        // For deferred batch destruction with callback, we push a generic command
        MassAPI->Defer().PushCommand<FMassDeferredDestroyCommand>([MassHandles, OnFinished](FMassEntityManager& Manager)
            {
                Manager.BatchDestroyEntities(MassHandles);

                // Fire delegate for each entity destroyed
                if (OnFinished.IsBound())
                {
                    for (const FMassEntityHandle& Handle : MassHandles)
                    {
                        // Note: The entity is now invalid in Mass, but we pass the handle back so BP knows which ID was destroyed.
                        OnFinished.Execute(FEntityHandle(Handle));
                    }
                }
            });
    }
    else if (FMassEntityManager* EntityManager = MassAPI->GetEntityManager())
    {
        EntityManager->BatchDestroyEntities(MassHandles);

        // Fire delegate for each entity
        if (OnFinished.IsBound())
        {
            for (const FEntityHandle& Handle : EntityHandles)
            {
                OnFinished.Execute(Handle);
            }
        }
    }
}

//================ Entity Building													    						========

FEntityHandle UMassAPIFuncLib::BuildEntityFromTemplateData(const UObject* WorldContextObject, UPARAM(ref) const FEntityTemplateData& TemplateData, const bool bDeferred, const FOnMassDeferredFinished OnFinished)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI) return FEntityHandle();

    // Make sure we have valid data
    FMassEntityTemplateData* Data = TemplateData.Get();
    if (!Data) return FEntityHandle();

    FMassEntityManager* Manager = MassAPI->GetEntityManager();
    checkf(Manager, TEXT("EntityManager is not available"));

    if (bDeferred)
    {
        // 1. Reserve immediately so we can return the handle
        const FMassEntityHandle ReservedEntity = Manager->ReserveEntity();

        // 2. Capture data for the command
        // Note: We copy the template descriptors to capture them by value for the lambda
        Data->Sort(); // Ensure sorted before capturing
        const FMassArchetypeCompositionDescriptor Composition = Data->GetCompositionDescriptor();
        const FMassArchetypeCreationParams CreationParams = Data->GetArchetypeCreationParams();
        const FMassArchetypeSharedFragmentValues SharedValues = Data->GetSharedFragmentValues();
        const TArray<FInstancedStruct> InitialFragments(Data->GetInitialFragmentValues());

        // 3. Push deferred creation command with callback
        MassAPI->Defer().PushCommand<FMassDeferredCreateCommand>([ReservedEntity, Composition, CreationParams, SharedValues, InitialFragments, OnFinished](FMassEntityManager& Manager)
            {
                // Get or create archetype
                const FMassArchetypeHandle ArchetypeHandle = Manager.CreateArchetype(Composition, CreationParams);
                if (!ArchetypeHandle.IsValid())
                {
                    Manager.ReleaseReservedEntity(ReservedEntity);
                    return;
                }

                // Build entity
                Manager.BuildEntity(ReservedEntity, ArchetypeHandle, SharedValues);

                // Set initial fragment values
                if (InitialFragments.Num() > 0)
                {
                    Manager.SetEntityFragmentValues(ReservedEntity, InitialFragments);
                }

                // Fire Callback
                OnFinished.ExecuteIfBound(ReservedEntity);
            });

        return FEntityHandle(ReservedEntity);
    }
    else
    {
        // Immediate build
        FMassEntityHandle MassHandle = MassAPI->BuildEntity(*Data);
        OnFinished.ExecuteIfBound(MassHandle);
        return FEntityHandle(MassHandle);
    }
}

TArray<FEntityHandle> UMassAPIFuncLib::BuildEntitiesFromTemplateData(const UObject* WorldContextObject, int32 Quantity, UPARAM(ref) const FEntityTemplateData& TemplateData, const bool bDeferred, const FOnMassDeferredFinished OnFinished)
{
    TArray<FEntityHandle> BPHandles;
    if (Quantity <= 0) return BPHandles;

    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI) return BPHandles;

    FMassEntityTemplateData* Data = TemplateData.Get();
    if (!Data) return BPHandles;

    FMassEntityManager* Manager = MassAPI->GetEntityManager();
    checkf(Manager, TEXT("EntityManager is not available"));

    if (bDeferred)
    {
        // 1. Batch reserve entities
        TArray<FMassEntityHandle> ReservedEntities;
        ReservedEntities.AddUninitialized(Quantity);
        Manager->BatchReserveEntities(MakeArrayView(ReservedEntities));

        // Populate return array
        BPHandles.Reserve(Quantity);
        for (const FMassEntityHandle& Handle : ReservedEntities)
        {
            BPHandles.Add(FEntityHandle(Handle));
        }

        // 2. Capture data
        Data->Sort();
        const FMassArchetypeCompositionDescriptor Composition = Data->GetCompositionDescriptor();
        const FMassArchetypeCreationParams CreationParams = Data->GetArchetypeCreationParams();
        const FMassArchetypeSharedFragmentValues SharedValues = Data->GetSharedFragmentValues();
        const TArray<FInstancedStruct> InitialFragments(Data->GetInitialFragmentValues());

        // 3. Push deferred command with callback loop
        MassAPI->Defer().PushCommand<FMassDeferredCreateCommand>([ReservedEntities, Composition, CreationParams, SharedValues, InitialFragments, OnFinished](FMassEntityManager& Manager)
            {
                const FMassArchetypeHandle ArchetypeHandle = Manager.CreateArchetype(Composition, CreationParams);
                if (!ArchetypeHandle.IsValid())
                {
                    for (const FMassEntityHandle& Entity : ReservedEntities)
                    {
                        Manager.ReleaseReservedEntity(Entity);
                    }
                    return;
                }

                Manager.BatchCreateReservedEntities(ArchetypeHandle, SharedValues, ReservedEntities);

                if (InitialFragments.Num() > 0)
                {
                    TArray<FMassArchetypeEntityCollection> EntityCollections;
                    UE::Mass::Utils::CreateEntityCollections(Manager, ReservedEntities, FMassArchetypeEntityCollection::NoDuplicates, EntityCollections);
                    Manager.BatchSetEntityFragmentValues(EntityCollections, InitialFragments);
                }

                // Fire callback for each entity
                if (OnFinished.IsBound())
                {
                    for (const FMassEntityHandle& Entity : ReservedEntities)
                    {
                        OnFinished.Execute(FEntityHandle(Entity));
                    }
                }
            });
    }
    else
    {
        // Immediate batch build
        TArray<FMassEntityHandle> MassHandles = MassAPI->BuildEntities(Quantity, *Data);

        BPHandles.Reserve(MassHandles.Num());
        for (const FMassEntityHandle& MassHandle : MassHandles)
        {
            BPHandles.Add(FEntityHandle(MassHandle));
        }

        // Fire callbacks
        if (OnFinished.IsBound())
        {
            for (const FEntityHandle& BPHandle : BPHandles)
            {
                OnFinished.Execute(BPHandle);
            }
        }
    }

    return BPHandles;
}

//================ Entity Querying & BP Processors												    			========

bool UMassAPIFuncLib::MatchEntityQuery(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(ref) const FEntityQuery& Query)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        return MassAPI->MatchQuery(EntityHandle, Query);
    }
    return false;
}

int32 UMassAPIFuncLib::GetNumMatchingEntities(const UObject* WorldContextObject, UPARAM(ref) const FEntityQuery& Query)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI) return 0;

    FMassEntityManager* EntityManager = MassAPI->GetEntityManager();
    if (!EntityManager) return 0;

    // 1. Create a bound Native Query (Registers with Manager immediately)
    FMassEntityQuery NativeQuery = Query.GetNativeQuery(EntityManager->AsShared());

    // 2. Flags Logic
    const int64 AllFlagsQuery = Query.GetAllFlagsBitmask();
    const int64 AnyFlagsQuery = Query.GetAnyFlagsBitmask();
    const int64 NoneFlagsQuery = Query.GetNoneFlagsBitmask();

    // 3. Fast Path
    if (AllFlagsQuery == 0 && AnyFlagsQuery == 0 && NoneFlagsQuery == 0)
    {
        return NativeQuery.GetNumMatchingEntities();
    }

    // 4. Slow Path
    int32 Count = 0;
    TArray<FMassEntityHandle> Matches = NativeQuery.GetMatchingEntityHandles();

    for (const FMassEntityHandle& Handle : Matches)
    {
        const FEntityFlagFragment* FlagFragment = EntityManager->GetFragmentDataPtr<FEntityFlagFragment>(Handle);
        const int64 EntityFlags = FlagFragment ? FlagFragment->Flags : 0;

        const bool bAll = (AllFlagsQuery == 0) || ((EntityFlags & AllFlagsQuery) == AllFlagsQuery);
        const bool bAny = (AnyFlagsQuery == 0) || ((EntityFlags & AnyFlagsQuery) != 0);
        const bool bNone = (NoneFlagsQuery == 0) || ((EntityFlags & NoneFlagsQuery) == 0);

        if (bAll && bAny && bNone)
        {
            Count++;
        }
    }

    return Count;
}

TArray<FEntityHandle> UMassAPIFuncLib::GetMatchingEntities(const UObject* WorldContextObject, UPARAM(ref) const FEntityQuery& Query)
{
    TArray<FEntityHandle> BPHandles;

    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI) return BPHandles;

    FMassEntityManager* EntityManager = MassAPI->GetEntityManager();
    if (!EntityManager) return BPHandles;

    // 1. Create Bound Query
    FMassEntityQuery NativeQuery = Query.GetNativeQuery(EntityManager->AsShared());

    // 2. Execute
    TArray<FMassEntityHandle> Matches = NativeQuery.GetMatchingEntityHandles();

    // 3. Flags Logic
    const int64 AllFlagsQuery = Query.GetAllFlagsBitmask();
    const int64 AnyFlagsQuery = Query.GetAnyFlagsBitmask();
    const int64 NoneFlagsQuery = Query.GetNoneFlagsBitmask();

    // 4. Fast Path
    if (AllFlagsQuery == 0 && AnyFlagsQuery == 0 && NoneFlagsQuery == 0)
    {
        BPHandles.Reserve(Matches.Num());
        for (const FMassEntityHandle& H : Matches)
        {
            BPHandles.Add(FEntityHandle(H));
        }
        return BPHandles;
    }

    // 5. Filter Path
    BPHandles.Reserve(Matches.Num());
    for (const FMassEntityHandle& Handle : Matches)
    {
        const FEntityFlagFragment* FlagFragment = EntityManager->GetFragmentDataPtr<FEntityFlagFragment>(Handle);
        const int64 EntityFlags = FlagFragment ? FlagFragment->Flags : 0;

        const bool bAll = (AllFlagsQuery == 0) || ((EntityFlags & AllFlagsQuery) == AllFlagsQuery);
        const bool bAny = (AnyFlagsQuery == 0) || ((EntityFlags & AnyFlagsQuery) != 0);
        const bool bNone = (NoneFlagsQuery == 0) || ((EntityFlags & NoneFlagsQuery) == 0);

        if (bAll && bAny && bNone)
        {
            BPHandles.Add(FEntityHandle(Handle));
        }
    }

    return BPHandles;
}

UMassAPISubsystem* UMassAPIFuncLib::ForEachMatchingEntities(const UObject* WorldContextObject)
{
    // 直接返回 Subsystem，不再使用静态缓存的 Helper
    return UMassAPISubsystem::GetPtr(WorldContextObject);
}

//================ TemplateData Operations														    			========

FEntityTemplateData UMassAPIFuncLib::GetTemplateData(const UObject* WorldContextObject, UPARAM(ref) const FEntityTemplate& Template)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        if (FMassEntityManager* EntityManager = MassAPI->GetEntityManager())
        {
            TSharedPtr<FMassEntityTemplateData> NewTemplateData = MakeShared<FMassEntityTemplateData>();
            Template.GetTemplateData(*NewTemplateData, *EntityManager);
            return FEntityTemplateData(NewTemplateData);
        }
    }
    return FEntityTemplateData();
}

FEntityTemplateData UMassAPIFuncLib::Conv_TemplateToTemplateData(const UObject* WorldContextObject, UPARAM(ref) const FEntityTemplate& Template)
{
    // This is the auto-cast conversion function
    // It simply calls the existing GetTemplateData function
    return GetTemplateData(WorldContextObject, Template);
}

bool UMassAPIFuncLib::IsEmpty_TemplateData(UPARAM(ref) const FEntityTemplateData& TemplateData)
{
    if (const FMassEntityTemplateData* Data = TemplateData.Get())
    {
        return Data->IsEmpty();
    }
    return true;
}


//================ Fragment Operations =========================================

//———————— Set.Fragment.Entity (Unified) ———————————————————————————————————————

void UMassAPIFuncLib::SetFragment_Entity_Unified(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const FGenericStruct& InFragment, const bool bDeferred, bool& bSuccess, FOnMassDeferredFinished OnFinished)
{
    checkNoEntry();
}

// [CHANGED] Signature now uses Value for OnFinished
void UMassAPIFuncLib::Generic_SetFragment_Entity_Unified(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const void* InFragmentPtr, bool bDeferred, bool& bSuccess, FOnMassDeferredFinished OnFinished)
{
    bSuccess = false;

    // ... (Validation Checks) ...
    if (!FragmentType || !InFragmentPtr) return;

    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !MassAPI->IsValid(EntityHandle)) return;

    FMassEntityManager& EntityManager = *MassAPI->GetEntityManager();

    if (FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        FInstancedStruct FragmentInstance;
        FragmentInstance.InitializeAs(FragmentType, static_cast<const uint8*>(InFragmentPtr));

        if (bDeferred)
        {
            // Capture by Value (The delegate itself is a small struct wrapping a shared pointer)
            MassAPI->Defer().PushCommand<FMassDeferredSetCommand>([EntityHandle, FragmentInstance, OnFinished](FMassEntityManager& Manager)
                {
                    if (Manager.IsEntityValid(EntityHandle))
                    {
                        Manager.AddFragmentInstanceListToEntity(EntityHandle, MakeArrayView(&FragmentInstance, 1));

                        // Execute safe
                        OnFinished.ExecuteIfBound(EntityHandle);
                    }
                });
            bSuccess = true;
        }
        else
        {
            if (MassAPI->HasFragment(EntityHandle, FragmentType))
            {
                EntityManager.SetEntityFragmentValues(EntityHandle, MakeArrayView(&FragmentInstance, 1));
            }
            else
            {
                MassAPI->AddFragment(EntityHandle, FragmentInstance);
            }
            bSuccess = true;
            OnFinished.ExecuteIfBound(EntityHandle);
        }
    }
    else if (FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        if (bDeferred)
        {
            UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetFragment_Entity: Deferred setting of Shared Fragments is not supported."));
            bSuccess = false;
        }
        else
        {
            if (MassAPI->HasFragment(EntityHandle, FragmentType))
            {
                EntityManager.RemoveSharedFragmentFromEntity(EntityHandle, *FragmentType);
            }
            const FSharedStruct& SharedStruct = EntityManager.GetOrCreateSharedFragment(*FragmentType, static_cast<const uint8*>(InFragmentPtr));
            bSuccess = EntityManager.AddSharedFragmentToEntity(EntityHandle, SharedStruct);
        }
    }
    else if (FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        if (bDeferred)
        {
            UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetFragment_Entity: Deferred setting of Const Shared Fragments is not supported."));
            bSuccess = false;
        }
        else
        {
            if (MassAPI->HasFragment(EntityHandle, FragmentType))
            {
                EntityManager.RemoveConstSharedFragmentFromEntity(EntityHandle, *FragmentType);
            }
            const FConstSharedStruct& ConstSharedStruct = EntityManager.GetOrCreateConstSharedFragment(*FragmentType, static_cast<const uint8*>(InFragmentPtr));
            bSuccess = EntityManager.AddConstSharedFragmentToEntity(EntityHandle, ConstSharedStruct);
        }
    }
    else
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetFragment_Entity: Unknown or unsupported fragment type '%s'."), *FragmentType->GetName());
    }
}

DEFINE_FUNCTION(UMassAPIFuncLib::execSetFragment_Entity_Unified)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityHandle, EntityHandle);
    P_GET_OBJECT(UScriptStruct, FragmentType);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;

    P_GET_UBOOL(bDeferred);
    P_GET_UBOOL_REF(bSuccess);

    // [CHANGED] Read Delegate Property by Value (FScriptDelegate)
    P_GET_PROPERTY(FDelegateProperty, OnFinished_ScriptDelegate);

    P_FINISH;

    // [CHANGED] Convert FScriptDelegate to FOnMassDeferredFinished (Binary compatible safe cast)
    // Dynamic Delegates in UE are wrappers around FScriptDelegate.
    FOnMassDeferredFinished OnFinished;
    (FScriptDelegate&)OnFinished = OnFinished_ScriptDelegate;

    bSuccess = false;

    P_NATIVE_BEGIN
        Generic_SetFragment_Entity_Unified(WorldContextObject, EntityHandle, FragmentType, InFragmentPtr, bDeferred, bSuccess, OnFinished);
    P_NATIVE_END
}

//———————— Set.Fragment.Template (Unified) —————————————————————————————————————

void UMassAPIFuncLib::SetFragment_Template_Unified(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const FGenericStruct& InFragment)
{
    checkNoEntry();
}

void UMassAPIFuncLib::Generic_SetFragment_Template_Unified(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const void* InFragmentPtr)
{
    if (!FragmentType)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetFragment_Template: FragmentType is null."));
        return;
    }
    if (!InFragmentPtr)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetFragment_Template: InFragmentPtr is null."));
        return;
    }

    const FMassEntityTemplateData* OldData = TemplateData.Get();
    // Templates are copy-on-write. We create a new one to modify.
    TSharedPtr<FMassEntityTemplateData> NewData = MakeShared<FMassEntityTemplateData>();

    // Helper to copy Tags and Chunk Fragments Definitions (structure only, no payload usually)
    auto CopyCommonData = [&](const FMassEntityTemplateData* Source)
        {
            if (!Source) return;
            NewData->SetTemplateName(Source->GetTemplateName());

            Source->GetTags().ExportTypes([&](const UScriptStruct* TagType) {
                NewData->AddTag(*TagType);
                return true;
                });

            Source->GetCompositionDescriptor().GET_CHUNK_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) {
                // Chunk fragments in templates are generally just types, not values
                return true;
                });
        };

    if (OldData)
    {
        CopyCommonData(OldData);
        const FMassArchetypeSharedFragmentValues& SharedValues = OldData->GetSharedFragmentValues();

        if (FragmentType->IsChildOf(FMassFragment::StaticStruct()))
        {
            // 1. Copy Fragment Types
            OldData->GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) {
                NewData->AddFragment(*Type);
                return true;
                });

            // 2. Copy Existing Instance Values (Skip the one being set)
            for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues())
            {
                if (Fragment.GetScriptStruct() != FragmentType)
                {
                    NewData->AddFragment(FConstStructView(Fragment));
                }
            }

            // 3. Copy Shared/Const Shared
            for (const FSharedStruct& S : SharedValues.GetSharedFragments()) { NewData->AddSharedFragment(S); }
            for (const FConstSharedStruct& C : SharedValues.GetConstSharedFragments()) { NewData->AddConstSharedFragment(C); }

            // 4. Add/Overwrite the Target Fragment Value
            NewData->AddFragment(FConstStructView(FragmentType, static_cast<const uint8*>(InFragmentPtr)));
        }
        else if (FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
        {
            // Copy regular fragments
            OldData->GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) { NewData->AddFragment(*Type); return true; });
            for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues()) { NewData->AddFragment(FConstStructView(Fragment)); }

            // Copy Shared (Skip target)
            for (const FSharedStruct& S : SharedValues.GetSharedFragments())
            {
                if (S.GetScriptStruct() != FragmentType) NewData->AddSharedFragment(S);
            }

            // Copy Const Shared
            for (const FConstSharedStruct& C : SharedValues.GetConstSharedFragments()) { NewData->AddConstSharedFragment(C); }

            // Add New Shared
            if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
            {
                const FSharedStruct& NewStruct = MassAPI->GetEntityManager()->GetOrCreateSharedFragment(*FragmentType, static_cast<const uint8*>(InFragmentPtr));
                NewData->AddSharedFragment(NewStruct);
            }
        }
        else if (FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
        {
            // Copy regular fragments
            OldData->GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) { NewData->AddFragment(*Type); return true; });
            for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues()) { NewData->AddFragment(FConstStructView(Fragment)); }

            // Copy Shared
            for (const FSharedStruct& S : SharedValues.GetSharedFragments()) { NewData->AddSharedFragment(S); }

            // Copy Const Shared (Skip target)
            for (const FConstSharedStruct& C : SharedValues.GetConstSharedFragments())
            {
                if (C.GetScriptStruct() != FragmentType) NewData->AddConstSharedFragment(C);
            }

            // Add New Const Shared
            if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
            {
                const FConstSharedStruct& NewStruct = MassAPI->GetEntityManager()->GetOrCreateConstSharedFragment(*FragmentType, static_cast<const uint8*>(InFragmentPtr));
                NewData->AddConstSharedFragment(NewStruct);
            }
        }
        else
        {
            UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetFragment_Template: Unsupported fragment type '%s'. Chunk fragments cannot be set by value in templates."), *FragmentType->GetName());
            return; // Abort update
        }
    }
    else
    {
        // Handle creating a fresh template if OldData was null
        if (FragmentType->IsChildOf(FMassFragment::StaticStruct()))
        {
            NewData->AddFragment(FConstStructView(FragmentType, static_cast<const uint8*>(InFragmentPtr)));
        }
        else if (FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
        {
            if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
            {
                const FSharedStruct& NewStruct = MassAPI->GetEntityManager()->GetOrCreateSharedFragment(*FragmentType, static_cast<const uint8*>(InFragmentPtr));
                NewData->AddSharedFragment(NewStruct);
            }
        }
        else if (FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
        {
            if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
            {
                const FConstSharedStruct& NewStruct = MassAPI->GetEntityManager()->GetOrCreateConstSharedFragment(*FragmentType, static_cast<const uint8*>(InFragmentPtr));
                NewData->AddConstSharedFragment(NewStruct);
            }
        }
        else
        {
            UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetFragment_Template: Unsupported fragment type '%s' for new template."), *FragmentType->GetName());
            return;
        }
    }

    TemplateData = FEntityTemplateData(NewData);
}

DEFINE_FUNCTION(UMassAPIFuncLib::execSetFragment_Template_Unified)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityTemplateData, TemplateData);
    P_GET_OBJECT(UScriptStruct, FragmentType);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_FINISH;

    P_NATIVE_BEGIN
        Generic_SetFragment_Template_Unified(WorldContextObject, TemplateData, FragmentType, InFragmentPtr);
    P_NATIVE_END
}

//———————— Get.Fragment.Entity (Unified) ———————————————————————————————————————

void UMassAPIFuncLib::GetFragment_Entity_Unified(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess)
{
    checkNoEntry();
}

void UMassAPIFuncLib::Generic_GetFragment_Entity_Unified(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, void* OutFragmentPtr, bool& bSuccess)
{
    bSuccess = false;

    // 1. Validations
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetFragment_Entity: MassAPISubsystem unavailable."));
        return;
    }

    if (!MassAPI->IsValid(EntityHandle))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetFragment_Entity: EntityHandle is invalid."));
        return;
    }

    if (!FragmentType)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetFragment_Entity: FragmentType is null."));
        return;
    }

    if (!OutFragmentPtr)
    {
        UE_LOG(LogMassBlueprintAPI, Error, TEXT("GetFragment_Entity: OutFragmentPtr is null. internal generic error."));
        return;
    }

    const FMassEntityManager& EntityManager = *MassAPI->GetEntityManager();

    // 2. Dispatch
    if (FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        // Supported: GetFragmentDataStruct
        FStructView FragmentView = EntityManager.GetFragmentDataStruct(EntityHandle, FragmentType);
        if (FragmentView.IsValid())
        {
            FragmentType->CopyScriptStruct(OutFragmentPtr, FragmentView.GetMemory());
            bSuccess = true;
        }
    }
    else if (FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        // Supported: GetSharedFragmentDataStruct
        FConstStructView FragmentView = EntityManager.GetSharedFragmentDataStruct(EntityHandle, FragmentType);
        if (FragmentView.IsValid())
        {
            FragmentType->CopyScriptStruct(OutFragmentPtr, FragmentView.GetMemory());
            bSuccess = true;
        }
    }
    else if (FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        // Supported: GetConstSharedFragmentDataStruct
        FConstStructView FragmentView = EntityManager.GetConstSharedFragmentDataStruct(EntityHandle, FragmentType);
        if (FragmentView.IsValid())
        {
            FragmentType->CopyScriptStruct(OutFragmentPtr, FragmentView.GetMemory());
            bSuccess = true;
        }
    }
    else if (FragmentType->IsChildOf(FMassChunkFragment::StaticStruct()))
    {
        // Not Supported: No API in MassEntityManager to get Chunk Data for a specific Entity Handle
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetFragment_Entity: Chunk Fragments '%s' cannot be retrieved via Entity Handle."), *FragmentType->GetName());
        bSuccess = false;
    }
    else
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetFragment_Entity: Unsupported fragment type '%s'."), *FragmentType->GetName());
    }
}

DEFINE_FUNCTION(UMassAPIFuncLib::execGetFragment_Entity_Unified)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityHandle, EntityHandle);
    P_GET_OBJECT(UScriptStruct, FragmentType);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    void* OutFragmentPtr = Stack.MostRecentPropertyAddress;
    P_GET_UBOOL_REF(bSuccess);
    P_FINISH;

    if (FragmentType && OutFragmentPtr)
    {
        // Initialize output memory before writing to it
        FragmentType->InitializeStruct(OutFragmentPtr);
    }
    bSuccess = false;

    P_NATIVE_BEGIN
        Generic_GetFragment_Entity_Unified(WorldContextObject, EntityHandle, FragmentType, OutFragmentPtr, bSuccess);
    P_NATIVE_END
}

//———————— Get.Fragment.Template (Unified) ———————————————————————————————————————————————

void UMassAPIFuncLib::GetFragment_Template_Unified(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess)
{
    checkNoEntry();
}

void UMassAPIFuncLib::Generic_GetFragment_Template_Unified(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, void* OutFragmentPtr, bool& bSuccess)
{
    bSuccess = false;

    if (!FragmentType)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetFragment_Template: FragmentType is null."));
        return;
    }
    if (!OutFragmentPtr)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetFragment_Template: OutFragmentPtr is null."));
        return;
    }

    const FMassEntityTemplateData* Data = TemplateData.Get();
    if (!Data)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetFragment_Template: TemplateData is empty/invalid."));
        return;
    }

    if (FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        const TConstArrayView<FInstancedStruct> InitialValues = Data->GetInitialFragmentValues();
        const FInstancedStruct* FoundFragment = InitialValues.FindByPredicate([FragmentType](const FInstancedStruct& Element) {
            return Element.GetScriptStruct() == FragmentType;
            });

        if (FoundFragment && FoundFragment->IsValid())
        {
            FragmentType->CopyScriptStruct(OutFragmentPtr, FoundFragment->GetMemory());
            bSuccess = true;
        }
    }
    else if (FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        const FMassArchetypeSharedFragmentValues& SharedValues = Data->GetSharedFragmentValues();
        const FSharedStruct* FoundShared = SharedValues.GetSharedFragments().FindByPredicate([FragmentType](const FSharedStruct& Element) {
            return Element.GetScriptStruct() == FragmentType;
            });

        if (FoundShared && FoundShared->IsValid())
        {
            FragmentType->CopyScriptStruct(OutFragmentPtr, FoundShared->GetMemory());
            bSuccess = true;
        }
    }
    else if (FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        const FMassArchetypeSharedFragmentValues& SharedValues = Data->GetSharedFragmentValues();
        const FConstSharedStruct* FoundConstShared = SharedValues.GetConstSharedFragments().FindByPredicate([FragmentType](const FConstSharedStruct& Element) {
            return Element.GetScriptStruct() == FragmentType;
            });

        if (FoundConstShared && FoundConstShared->IsValid())
        {
            FragmentType->CopyScriptStruct(OutFragmentPtr, FoundConstShared->GetMemory());
            bSuccess = true;
        }
    }
    else if (FragmentType->IsChildOf(FMassChunkFragment::StaticStruct()))
    {
        // Chunk Fragments and Tags do not store "values" in the template that can be retrieved by pointer.
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetFragment_Template: Chunk Fragments '%s' cannot be retrieved via TemplateData"), *FragmentType->GetName());
    }
    else
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetFragment_Template: Unsupported fragment type '%s'."), *FragmentType->GetName());
    }
}

DEFINE_FUNCTION(UMassAPIFuncLib::execGetFragment_Template_Unified)
{
    P_GET_STRUCT_REF(FEntityTemplateData, TemplateData);
    P_GET_OBJECT(UScriptStruct, FragmentType);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    void* OutFragmentPtr = Stack.MostRecentPropertyAddress;
    P_GET_UBOOL_REF(bSuccess);
    P_FINISH;

    if (FragmentType && OutFragmentPtr)
    {
        FragmentType->InitializeStruct(OutFragmentPtr);
    }
    bSuccess = false;

    P_NATIVE_BEGIN
        Generic_GetFragment_Template_Unified(TemplateData, FragmentType, OutFragmentPtr, bSuccess);
    P_NATIVE_END
}

//———————— Remove.Fragment.Entity (Unified) ————————————————————————————————————

bool UMassAPIFuncLib::RemoveFragment_Entity_Unified(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, bool bDeferred, FOnMassDeferredFinished OnFinished)
{
    if (!FragmentType)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveFragment_Entity: FragmentType is null"));
        return false;
    }

    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !MassAPI->IsValid(EntityHandle))
    {
        return false;
    }

    if (bDeferred)
    {
        if (FragmentType->IsChildOf(FMassFragment::StaticStruct()))
        {
            // Explicitly use FMassDeferredRemoveCommand for removing fragments deferredly
            MassAPI->Defer().PushCommand<FMassDeferredRemoveCommand>([EntityHandle, FragmentType, OnFinished](FMassEntityManager& Manager)
                {
                    if (Manager.IsEntityValid(EntityHandle))
                    {
                        Manager.RemoveFragmentFromEntity(EntityHandle, FragmentType);
                        OnFinished.ExecuteIfBound(EntityHandle);
                    }
                });
            return true;
        }
        else if (FragmentType->IsChildOf(FMassTag::StaticStruct()))
        {
            // Explicitly use FMassDeferredRemoveCommand for removing tags deferredly
            MassAPI->Defer().PushCommand<FMassDeferredRemoveCommand>([EntityHandle, FragmentType, OnFinished](FMassEntityManager& Manager)
                {
                    if (Manager.IsEntityValid(EntityHandle))
                    {
                        Manager.RemoveTagFromEntity(EntityHandle, FragmentType);
                        OnFinished.ExecuteIfBound(EntityHandle);
                    }
                });
            return true;
        }
        else
        {
            UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveFragment_Entity (Deferred): Only Fragments and Tags are fully supported deferred. Shared/ConstShared fragments cannot be removed deferred via this node."));
            return false;
        }
    }
    else
    {
        bool bResult = false;
        if (FragmentType->IsChildOf(FMassFragment::StaticStruct()))
        {
            MassAPI->RemoveFragment(EntityHandle, FragmentType);
            bResult = true;
        }
        else if (FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
        {
            bResult = MassAPI->RemoveSharedFragment(EntityHandle, FragmentType);
        }
        else if (FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
        {
            bResult = MassAPI->RemoveConstSharedFragment(EntityHandle, FragmentType);
        }
        else if (FragmentType->IsChildOf(FMassChunkFragment::StaticStruct()))
        {
            UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveFragment_Entity: Cannot remove Chunk Fragment '%s'. Only Tag, Fragment, and SharedFragment are supported."), *FragmentType->GetName());
            bResult = false;
        }
        else
        {
            UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveFragment_Entity: Unknown or unsupported fragment type '%s'"), *FragmentType->GetName());
            bResult = false;
        }

        if (bResult)
        {
            OnFinished.ExecuteIfBound(EntityHandle);
        }
        return bResult;
    }
}

//———————— Remove.Fragment.Template (Unified) ——————————————————————————————————

void UMassAPIFuncLib::RemoveFragment_Template_Unified(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    if (!FragmentType)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveFragment_Template: FragmentType is null"));
        return;
    }

    const FMassEntityTemplateData* OldData = TemplateData.Get();
    if (!OldData)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveFragment_Template: TemplateData is empty."));
        return;
    }

    TSharedPtr<FMassEntityTemplateData> NewData = MakeShared<FMassEntityTemplateData>();
    NewData->SetTemplateName(OldData->GetTemplateName());

    const FMassArchetypeSharedFragmentValues& SharedValues = OldData->GetSharedFragmentValues();

    // Copy Tags (Tags are not removed by this function)
    OldData->GetTags().ExportTypes([&](const UScriptStruct* T) {
        NewData->AddTag(*T);
        return true;
        });

    // Copy Chunk Fragments (Unless it is the type being removed)
    OldData->GetCompositionDescriptor().GET_CHUNK_FRAGMENTS.ExportTypes([&](const UScriptStruct* T) {
        if (T != FragmentType) NewData->AddFragment(*T); // AddFragment in template handles chunks by descriptor
        return true;
        });

    // Logic: Copy everything EXCEPT what matches FragmentType
    if (FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        // Copy Types (Exclude Target)
        OldData->GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type)
            {
                if (Type != FragmentType) NewData->AddFragment(*Type);
                return true;
            });

        // Copy Values (Exclude Target)
        for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues())
        {
            if (Fragment.GetScriptStruct() != FragmentType) NewData->AddFragment(FConstStructView(Fragment));
        }

        // Copy all shared
        for (const FSharedStruct& S : SharedValues.GetSharedFragments()) { NewData->AddSharedFragment(S); }
        for (const FConstSharedStruct& C : SharedValues.GetConstSharedFragments()) { NewData->AddConstSharedFragment(C); }
    }
    else if (FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        // Copy all fragments
        OldData->GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) { NewData->AddFragment(*Type); return true; });
        for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues()) { NewData->AddFragment(FConstStructView(Fragment)); }

        // Copy Shared (Exclude Target)
        for (const FSharedStruct& S : SharedValues.GetSharedFragments())
        {
            if (S.GetScriptStruct() != FragmentType) NewData->AddSharedFragment(S);
        }

        // Copy all Const
        for (const FConstSharedStruct& C : SharedValues.GetConstSharedFragments()) { NewData->AddConstSharedFragment(C); }
    }
    else if (FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        // Copy all fragments
        OldData->GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) { NewData->AddFragment(*Type); return true; });
        for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues()) { NewData->AddFragment(FConstStructView(Fragment)); }

        // Copy all Shared
        for (const FSharedStruct& S : SharedValues.GetSharedFragments()) { NewData->AddSharedFragment(S); }

        // Copy Const (Exclude Target)
        for (const FConstSharedStruct& C : SharedValues.GetConstSharedFragments())
        {
            if (C.GetScriptStruct() != FragmentType) NewData->AddConstSharedFragment(C);
        }
    }
    else if (FragmentType->IsChildOf(FMassChunkFragment::StaticStruct()))
    {
        // Logic was already handled in the 'Copy Chunk Fragments' loop above.
    }
    else
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveFragment_Template: Unsupported type '%s'"), *FragmentType->GetName());
        return;
    }

    TemplateData = FEntityTemplateData(NewData);
}

//———————— Has.Fragment.Entity —————————————————————————————————————————————————

bool UMassAPIFuncLib::HasFragment_Entity_Unified(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType)
{
    if (!FragmentType)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("HasFragment_Entity: FragmentType is null"));
        return false;
    }

    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("HasFragment_Entity: MassAPISubsystem not found"));
        return false;
    }

    if (!MassAPI->IsValid(EntityHandle))
    {
        return false;
    }

    const FMassEntityManager& Manager = *MassAPI->GetEntityManager();

    // 1. Get Archetype (Fast)
    FMassArchetypeHandle Archetype = Manager.GetArchetypeForEntity(EntityHandle);
    if (!Archetype.IsValid())
    {
        return false;
    }

    // 2. Get Composition (Fast)
    const FMassArchetypeCompositionDescriptor& Composition = Manager.GetArchetypeComposition(Archetype);

    // 3. Check Bitsets based on type
    if (FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        return Composition.GET_FRAGMENTS.Contains(*FragmentType);
    }
    else if (FragmentType->IsChildOf(FMassTag::StaticStruct()))
    {
        return Composition.GET_TAGS.Contains(*FragmentType);
    }
    else if (FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        return Composition.GET_SHARED_FRAGMENTS.Contains(*FragmentType);
    }
    else if (FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        return Composition.GET_CONST_SHARED_FRAGMENTS.Contains(*FragmentType);
    }
    else if (FragmentType->IsChildOf(FMassChunkFragment::StaticStruct()))
    {
        return Composition.GET_CHUNK_FRAGMENTS.Contains(*FragmentType);
    }
    else
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("HasFragment_Entity: Unknown or unsupported type '%s'"), *FragmentType->GetName());
        return false;
    }
}

//———————— Has.Fragment.Template ———————————————————————————————————————————————

bool UMassAPIFuncLib::HasFragment_Template_Unified(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    if (!FragmentType)
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("HasFragment_Template: FragmentType is null"));
        return false;
    }

    if (const FMassEntityTemplateData* Data = TemplateData.Get())
    {
        if (FragmentType->IsChildOf(FMassFragment::StaticStruct()))
        {
            return Data->HasFragment(*FragmentType);
        }
        else if (FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
        {
            return Data->HasSharedFragment(*FragmentType);
        }
        else if (FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
        {
            return Data->HasConstSharedFragment(*FragmentType);
        }
        else if (FragmentType->IsChildOf(FMassChunkFragment::StaticStruct()))
        {
            return Data->GetCompositionDescriptor().GET_CHUNK_FRAGMENTS.Contains(*FragmentType);
        }
        else
        {
            UE_LOG(LogMassBlueprintAPI, Warning, TEXT("HasFragment_Template: Unknown type '%s'"), *FragmentType->GetName());
            return false;
        }
    }

    UE_LOG(LogMassBlueprintAPI, Warning, TEXT("HasFragment_Template: TemplateData is empty"));
    return false;
}

//================ Tag Operations																	    		========

//———————— Add.Tag.Entity																					    			————

void UMassAPIFuncLib::AddTag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType, bool bDeferred, FOnMassDeferredFinished OnFinished)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !MassAPI->IsValid(EntityHandle) || !TagType) return;

    if (!TagType->IsChildOf(FMassTag::StaticStruct())) return;

    if (bDeferred)
    {
        MassAPI->Defer().PushCommand<FMassDeferredAddCommand>([EntityHandle, TagType, OnFinished](FMassEntityManager& Manager)
            {
                if (Manager.IsEntityValid(EntityHandle))
                {
                    Manager.AddTagToEntity(EntityHandle, TagType);
                    OnFinished.ExecuteIfBound(EntityHandle);
                }
            });
    }
    else
    {
        MassAPI->GetEntityManager()->AddTagToEntity(EntityHandle, TagType);
        OnFinished.ExecuteIfBound(EntityHandle);
    }
}

//———————— Add.Tag.Template																	    					————

void UMassAPIFuncLib::AddTag_Template(UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* TagType)
{
    if (FMassEntityTemplateData* Data = TemplateData.Get())
    {
        if (TagType && TagType->IsChildOf(FMassTag::StaticStruct()))
        {
            Data->AddTag(*TagType);
        }
    }
}

//———————— Remove.Tag.Entity																						    	————

void UMassAPIFuncLib::RemoveTag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType, bool bDeferred, FOnMassDeferredFinished OnFinished)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !MassAPI->IsValid(EntityHandle) || !TagType) return;

    if (!TagType->IsChildOf(FMassTag::StaticStruct())) return;

    if (bDeferred)
    {
        MassAPI->Defer().PushCommand<FMassDeferredRemoveCommand>([EntityHandle, TagType, OnFinished](FMassEntityManager& Manager)
            {
                if (Manager.IsEntityValid(EntityHandle))
                {
                    Manager.RemoveTagFromEntity(EntityHandle, TagType);
                    OnFinished.ExecuteIfBound(EntityHandle);
                }
            });
    }
    else
    {
        MassAPI->GetEntityManager()->RemoveTagFromEntity(EntityHandle, TagType);
        OnFinished.ExecuteIfBound(EntityHandle);
    }
}

//———————— Remove.Tag.Template																			    		————

void UMassAPIFuncLib::RemoveTag_Template(UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* TagType)
{
    if (FMassEntityTemplateData* Data = TemplateData.Get())
    {
        if (TagType && TagType->IsChildOf(FMassTag::StaticStruct()))
        {
            Data->RemoveTag(*TagType);
        }
    }
}

//———————— Has.Tag.Entity																						           	————

bool UMassAPIFuncLib::HasTag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        return MassAPI->HasTag(EntityHandle, TagType);
    }
    return false;
}

//———————— Has.Tag.Template																				    		————

bool UMassAPIFuncLib::HasTag_Template(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* TagType)
{
    if (const FMassEntityTemplateData* Data = TemplateData.Get())
    {
        if (TagType && TagType->IsChildOf(FMassTag::StaticStruct()))
        {
            return Data->GetCompositionDescriptor().GET_TAGS.Contains(*TagType);
        }
    }
    return false;
}

//================ Flag Operations																		    	========

//———————— Set.Flag																							    	————

bool UMassAPIFuncLib::SetFlag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToSet)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (MassAPI)
    {
        return MassAPI->SetEntityFlag(EntityHandle, FlagToSet);
    }
    return false;
}

//———————— Set.Flag.Template																				    	————

void UMassAPIFuncLib::SetFlag_Template(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, EEntityFlags FlagToSet)
{
    if (FlagToSet >= EEntityFlags::EEntityFlags_MAX)
    {
        return;
    }

    // 1. Get current flags state
    const int64 CurrentFlags = GetFlag_Template(TemplateData);

    // 2. Create a temporary fragment to utilize its methods
    FEntityFlagFragment TempFlagFragment;
    TempFlagFragment.Flags = CurrentFlags;

    // 3. Use the fragment's own logic to set the flag
    TempFlagFragment.SetFlag(FlagToSet);

    // 4. Write the modified fragment back to the template
    Generic_SetFragment_Template_Unified(WorldContextObject, TemplateData, FEntityFlagFragment::StaticStruct(), &TempFlagFragment);
}

//———————— Get.Flags																					    		————

int64 UMassAPIFuncLib::GetFlag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI)
    {
        return 0;
    }
    return MassAPI->GetEntityFlags(EntityHandle);
}

//———————— Get.Flags.Template																			    		————

int64 UMassAPIFuncLib::GetFlag_Template(UPARAM(ref) const FEntityTemplateData& TemplateData)
{
    if (const FMassEntityTemplateData* Data = TemplateData.Get())
    {
        TConstArrayView<FInstancedStruct> InitialValues = Data->GetInitialFragmentValues();
        for (const FInstancedStruct& Value : InitialValues)
        {
            if (Value.GetScriptStruct() == FEntityFlagFragment::StaticStruct())
            {
                if (const FEntityFlagFragment* FlagFragment = reinterpret_cast<const FEntityFlagFragment*>(Value.GetMemory()))
                {
                    return FlagFragment->Flags;
                }
            }
        }
    }
    return 0;
}

//———————— Clear.Flag																				    			————

bool UMassAPIFuncLib::ClearFlag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToClear)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (MassAPI)
    {
        return MassAPI->ClearEntityFlag(EntityHandle, FlagToClear);
    }
    return false;
}

//———————— Clear.Flag.Template																		    			————

void UMassAPIFuncLib::ClearFlag_Template(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, EEntityFlags FlagToClear)
{
    if (FlagToClear >= EEntityFlags::EEntityFlags_MAX)
    {
        return;
    }

    // 1. Get current flags state
    const int64 CurrentFlags = GetFlag_Template(TemplateData);

    // 2. Create a temporary fragment to utilize its methods
    FEntityFlagFragment TempFlagFragment;
    TempFlagFragment.Flags = CurrentFlags;

    // 3. Use the fragment's own logic to clear the flag
    TempFlagFragment.ClearFlag(FlagToClear);

    // 4. Write the modified fragment back to the template
    Generic_SetFragment_Template_Unified(WorldContextObject, TemplateData, FEntityFlagFragment::StaticStruct(), &TempFlagFragment);
}

//———————— Has.Flag																					    			————

bool UMassAPIFuncLib::HasFlag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToTest)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        return MassAPI->HasEntityFlag(EntityHandle, FlagToTest);
    }
    return false;
}

//———————— Has.Flag.Template																		    			————

bool UMassAPIFuncLib::HasFlag_Template(UPARAM(ref) const FEntityTemplateData& TemplateData, EEntityFlags FlagToTest)
{
    if (FlagToTest >= EEntityFlags::EEntityFlags_MAX)
    {
        return false;
    }

    if (const FMassEntityTemplateData* Data = TemplateData.Get())
    {
        TConstArrayView<FInstancedStruct> InitialValues = Data->GetInitialFragmentValues();

        // Find the specific fragment manually to cast and call its method
        for (const FInstancedStruct& Value : InitialValues)
        {
            if (Value.GetScriptStruct() == FEntityFlagFragment::StaticStruct())
            {
                if (const FEntityFlagFragment* FlagFragment = reinterpret_cast<const FEntityFlagFragment*>(Value.GetMemory()))
                {
                    // Delegate to the fragment's check method
                    return FlagFragment->HasFlag(FlagToTest);
                }
            }
        }
    }

    return false;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Deprecated Functions
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//———————————————————Tag Ops

bool UMassAPIFuncLib::HasTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType)
{
    if (!TagType || !TagType->IsChildOf(FMassTag::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("HasTag: Type '%s' is not a child of FMassTag."), TagType ? *TagType->GetName() : TEXT("Null"));
        return false;
    }
    return HasTag_Entity(WorldContextObject, EntityHandle, TagType);
}

void UMassAPIFuncLib::AddTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType)
{
    if (!TagType || !TagType->IsChildOf(FMassTag::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("AddTag: Type '%s' is not a child of FMassTag."), TagType ? *TagType->GetName() : TEXT("Null"));
        return;
    }
    FOnMassDeferredFinished NoCallback;
    AddTag_Entity(WorldContextObject, EntityHandle, TagType, false, NoCallback);
}

void UMassAPIFuncLib::RemoveTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType)
{
    if (!TagType || !TagType->IsChildOf(FMassTag::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveTag: Type '%s' is not a child of FMassTag."), TagType ? *TagType->GetName() : TEXT("Null"));
        return;
    }
    FOnMassDeferredFinished NoCallback;
    RemoveTag_Entity(WorldContextObject, EntityHandle, TagType, false, NoCallback);
}

bool UMassAPIFuncLib::HasTag_TemplateData(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* TagType)
{
    if (!TagType || !TagType->IsChildOf(FMassTag::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("HasTag_TemplateData: Type '%s' is not a child of FMassTag."), TagType ? *TagType->GetName() : TEXT("Null"));
        return false;
    }
    return HasTag_Template(TemplateData, TagType);
}

void UMassAPIFuncLib::AddTag_TemplateData(UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* TagType)
{
    if (!TagType || !TagType->IsChildOf(FMassTag::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("AddTag_TemplateData: Type '%s' is not a child of FMassTag."), TagType ? *TagType->GetName() : TEXT("Null"));
        return;
    }
    AddTag_Template(TemplateData, TagType);
}

void UMassAPIFuncLib::RemoveTag_TemplateData(UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* TagType)
{
    if (!TagType || !TagType->IsChildOf(FMassTag::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveTag_TemplateData: Type '%s' is not a child of FMassTag."), TagType ? *TagType->GetName() : TEXT("Null"));
        return;
    }
    RemoveTag_Template(TemplateData, TagType);
}

//———————————————————Fragment Ops

bool UMassAPIFuncLib::HasFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType)
{
    if (!FragmentType || !FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("HasFragment: Type '%s' is not a child of FMassFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        return false;
    }
    return HasFragment_Entity_Unified(WorldContextObject, EntityHandle, FragmentType);
}

bool UMassAPIFuncLib::RemoveFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType)
{
    if (!FragmentType || !FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveFragment: Type '%s' is not a child of FMassFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        return false;
    }
    FOnMassDeferredFinished NoCallback;
    return RemoveFragment_Entity_Unified(WorldContextObject, EntityHandle, FragmentType, false, NoCallback);
}

void UMassAPIFuncLib::GetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess)
{
    checkNoEntry();
}

DEFINE_FUNCTION(UMassAPIFuncLib::execGetFragment)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityHandle, EntityHandle);
    P_GET_OBJECT(UScriptStruct, FragmentType);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    void* OutFragmentPtr = Stack.MostRecentPropertyAddress;
    P_GET_UBOOL_REF(bSuccess);
    P_FINISH;

    if (!FragmentType || !FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetFragment: Type '%s' is not a child of FMassFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        bSuccess = false;
        return;
    }

    if (OutFragmentPtr)
    {
        FragmentType->InitializeStruct(OutFragmentPtr);
    }
    bSuccess = false;

    P_NATIVE_BEGIN
        Generic_GetFragment_Entity_Unified(WorldContextObject, EntityHandle, FragmentType, OutFragmentPtr, bSuccess);
    P_NATIVE_END
}

void UMassAPIFuncLib::SetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess)
{
    checkNoEntry();
}

DEFINE_FUNCTION(UMassAPIFuncLib::execSetFragment)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityHandle, EntityHandle);
    P_GET_OBJECT(UScriptStruct, FragmentType);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_GET_UBOOL_REF(bSuccess);
    P_FINISH;

    if (!FragmentType || !FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetFragment: Type '%s' is not a child of FMassFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        bSuccess = false;
        return;
    }

    bSuccess = false;
    FOnMassDeferredFinished NoCallback;

    P_NATIVE_BEGIN
        Generic_SetFragment_Entity_Unified(WorldContextObject, EntityHandle, FragmentType, InFragmentPtr, false, bSuccess, NoCallback);
    P_NATIVE_END
}

bool UMassAPIFuncLib::HasFragment_TemplateData(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    if (!FragmentType || !FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("HasFragment_TemplateData: Type '%s' is not a child of FMassFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        return false;
    }
    return HasFragment_Template_Unified(TemplateData, FragmentType);
}

void UMassAPIFuncLib::RemoveFragmentInTemplate(UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    if (!FragmentType || !FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveFragmentInTemplate: Type '%s' is not a child of FMassFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        return;
    }
    RemoveFragment_Template_Unified(nullptr, TemplateData, FragmentType);
}

void UMassAPIFuncLib::GetFragmentFromTemplate(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess)
{
    GetFragment_Template_Unified(TemplateData, FragmentType, OutFragment, bSuccess);
}

DEFINE_FUNCTION(UMassAPIFuncLib::execGetFragmentFromTemplate)
{
    P_GET_STRUCT_REF(FEntityTemplateData, TemplateData);
    P_GET_OBJECT(UScriptStruct, FragmentType);
    Stack.StepCompiledIn<FStructProperty>(nullptr);
    void* OutFragmentPtr = Stack.MostRecentPropertyAddress;
    P_GET_UBOOL_REF(bSuccess);
    P_FINISH;

    if (!FragmentType || !FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetFragmentFromTemplate: Type '%s' is not a child of FMassFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        bSuccess = false;
        return;
    }

    if (OutFragmentPtr) FragmentType->InitializeStruct(OutFragmentPtr);
    bSuccess = false;
    P_NATIVE_BEGIN
        Generic_GetFragment_Template_Unified(TemplateData, FragmentType, OutFragmentPtr, bSuccess);
    P_NATIVE_END
}

void UMassAPIFuncLib::SetFragmentInTemplate(UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const FGenericStruct& InFragment)
{
    SetFragment_Template_Unified(nullptr, TemplateData, FragmentType, InFragment);
}

DEFINE_FUNCTION(UMassAPIFuncLib::execSetFragmentInTemplate)
{
    P_GET_STRUCT_REF(FEntityTemplateData, TemplateData);
    P_GET_OBJECT(UScriptStruct, FragmentType);
    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_FINISH;

    if (!FragmentType || !FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetFragmentInTemplate: Type '%s' is not a child of FMassFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        return;
    }

    P_NATIVE_BEGIN
        Generic_SetFragment_Template_Unified(nullptr, TemplateData, FragmentType, InFragmentPtr);
    P_NATIVE_END
}


//———————————————————SharedFragment Ops

// Missing HasSharedFragment

bool UMassAPIFuncLib::RemoveSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType)
{
    if (!FragmentType || !FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveSharedFragment: Type '%s' is not a child of FMassSharedFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        return false;
    }
    FOnMassDeferredFinished NoCallback;
    return RemoveFragment_Entity_Unified(WorldContextObject, EntityHandle, FragmentType, false, NoCallback);
}

void UMassAPIFuncLib::GetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess)
{
    checkNoEntry();
}

DEFINE_FUNCTION(UMassAPIFuncLib::execGetSharedFragment)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityHandle, EntityHandle);
    P_GET_OBJECT(UScriptStruct, FragmentType);
    Stack.StepCompiledIn<FStructProperty>(nullptr);
    void* OutFragmentPtr = Stack.MostRecentPropertyAddress;
    P_GET_UBOOL_REF(bSuccess);
    P_FINISH;

    if (!FragmentType || !FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetSharedFragment: Type '%s' is not a child of FMassSharedFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        bSuccess = false;
        return;
    }

    if (OutFragmentPtr) FragmentType->InitializeStruct(OutFragmentPtr);
    bSuccess = false;
    P_NATIVE_BEGIN
        Generic_GetFragment_Entity_Unified(WorldContextObject, EntityHandle, FragmentType, OutFragmentPtr, bSuccess);
    P_NATIVE_END
}

void UMassAPIFuncLib::SetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess)
{
    checkNoEntry();
}

DEFINE_FUNCTION(UMassAPIFuncLib::execSetSharedFragment)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityHandle, EntityHandle);
    P_GET_OBJECT(UScriptStruct, FragmentType);
    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_GET_UBOOL_REF(bSuccess);
    P_FINISH;

    if (!FragmentType || !FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetSharedFragment: Type '%s' is not a child of FMassSharedFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        bSuccess = false;
        return;
    }

    bSuccess = false;
    FOnMassDeferredFinished NoCallback;

    P_NATIVE_BEGIN
        Generic_SetFragment_Entity_Unified(WorldContextObject, EntityHandle, FragmentType, InFragmentPtr, false, bSuccess, NoCallback);
    P_NATIVE_END
}

// Missing HasSharedFragmentInTemplate

void UMassAPIFuncLib::RemoveSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    if (!FragmentType || !FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveSharedFragmentInTemplate: Type '%s' is not a child of FMassSharedFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        return;
    }
    RemoveFragment_Template_Unified(WorldContextObject, TemplateData, FragmentType);
}

// Missing GetSharedFragmentInTemplate

void UMassAPIFuncLib::SetSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref)FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const FGenericStruct& InFragment)
{
    checkNoEntry();
}

DEFINE_FUNCTION(UMassAPIFuncLib::execSetSharedFragmentInTemplate)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityTemplateData, TemplateData);
    P_GET_OBJECT(UScriptStruct, FragmentType);
    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_FINISH;

    if (!FragmentType || !FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetSharedFragmentInTemplate: Type '%s' is not a child of FMassSharedFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        return;
    }

    P_NATIVE_BEGIN
        Generic_SetFragment_Template_Unified(WorldContextObject, TemplateData, FragmentType, InFragmentPtr);
    P_NATIVE_END
}


//———————————————————ConstSharedFragment Ops

// Missing HasConstSharedFragment

bool UMassAPIFuncLib::RemoveConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType)
{
    if (!FragmentType || !FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveConstSharedFragment: Type '%s' is not a child of FMassConstSharedFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        return false;
    }
    FOnMassDeferredFinished NoCallback;
    return RemoveFragment_Entity_Unified(WorldContextObject, EntityHandle, FragmentType, false, NoCallback);
}

void UMassAPIFuncLib::GetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess)
{
    checkNoEntry();
}

DEFINE_FUNCTION(UMassAPIFuncLib::execGetConstSharedFragment)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityHandle, EntityHandle);
    P_GET_OBJECT(UScriptStruct, FragmentType);
    Stack.StepCompiledIn<FStructProperty>(nullptr);
    void* OutFragmentPtr = Stack.MostRecentPropertyAddress;
    P_GET_UBOOL_REF(bSuccess);
    P_FINISH;

    if (!FragmentType || !FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("GetConstSharedFragment: Type '%s' is not a child of FMassConstSharedFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        bSuccess = false;
        return;
    }

    if (OutFragmentPtr) FragmentType->InitializeStruct(OutFragmentPtr);
    bSuccess = false;
    P_NATIVE_BEGIN
        Generic_GetFragment_Entity_Unified(WorldContextObject, EntityHandle, FragmentType, OutFragmentPtr, bSuccess);
    P_NATIVE_END
}

void UMassAPIFuncLib::SetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess)
{
    checkNoEntry();
}

DEFINE_FUNCTION(UMassAPIFuncLib::execSetConstSharedFragment)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityHandle, EntityHandle);
    P_GET_OBJECT(UScriptStruct, FragmentType);
    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_GET_UBOOL_REF(bSuccess);
    P_FINISH;

    if (!FragmentType || !FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetConstSharedFragment: Type '%s' is not a child of FMassConstSharedFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        bSuccess = false;
        return;
    }

    bSuccess = false;
    FOnMassDeferredFinished NoCallback;

    P_NATIVE_BEGIN
        Generic_SetFragment_Entity_Unified(WorldContextObject, EntityHandle, FragmentType, InFragmentPtr, false, bSuccess, NoCallback);
    P_NATIVE_END
}

// Missing HasSharedFragmentInTemplate

void UMassAPIFuncLib::RemoveConstSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    if (!FragmentType || !FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("RemoveConstSharedFragmentInTemplate: Type '%s' is not a child of FMassConstSharedFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        return;
    }
    RemoveFragment_Template_Unified(WorldContextObject, TemplateData, FragmentType);
}

// Missing GetSharedFragmentInTemplate

void UMassAPIFuncLib::SetConstSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const FGenericStruct& InFragment)
{
    checkNoEntry();
}

DEFINE_FUNCTION(UMassAPIFuncLib::execSetConstSharedFragmentInTemplate)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityTemplateData, TemplateData);
    P_GET_OBJECT(UScriptStruct, FragmentType);
    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_FINISH;

    if (!FragmentType || !FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        UE_LOG(LogMassBlueprintAPI, Warning, TEXT("SetConstSharedFragmentInTemplate: Type '%s' is not a child of FMassConstSharedFragment."), FragmentType ? *FragmentType->GetName() : TEXT("Null"));
        return;
    }

    P_NATIVE_BEGIN
        Generic_SetFragment_Template_Unified(WorldContextObject, TemplateData, FragmentType, InFragmentPtr);
    P_NATIVE_END
}

//———————————————————Flag Ops

bool UMassAPIFuncLib::HasEntityFlag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToTest)
{
    return HasFlag_Entity(WorldContextObject, EntityHandle, FlagToTest);
}

bool UMassAPIFuncLib::ClearEntityFlag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToClear)
{
    return ClearFlag_Entity(WorldContextObject, EntityHandle, FlagToClear);
}

bool UMassAPIFuncLib::SetEntityFlag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToSet)
{
    return SetFlag_Entity(WorldContextObject, EntityHandle, FlagToSet);
}

bool UMassAPIFuncLib::HasTemplateFlag(UPARAM(ref) const FEntityTemplateData& TemplateData, EEntityFlags FlagToTest)
{
    return HasFlag_Template(TemplateData, FlagToTest);
}

void UMassAPIFuncLib::ClearTemplateFlag(UPARAM(ref) FEntityTemplateData& TemplateData, EEntityFlags FlagToClear)
{
    ClearFlag_Template(nullptr, TemplateData, FlagToClear);
}

void UMassAPIFuncLib::SetTemplateFlag(UPARAM(ref) FEntityTemplateData& TemplateData, EEntityFlags FlagToSet)
{
    SetFlag_Template(nullptr, TemplateData, FlagToSet);
}