#include "MassAPIBPFnLib.h"
#include "MassAPISubsystem.h"
#include "MassEntityView.h"
#include "MassEntitySubsystem.h"
#include "UObject/StructOnScope.h"
#include "MassEntityQuery.h" // Correct include
#include "MassRequirements.h" // Include for EMassFragmentPresence, FMassFragmentRequirements methods
#include "MassExternalSubsystemTraits.h" // Include for query subsystem requirements if needed
#include "MassDebugger.h" // Include for logging query validity if needed
#include "HAL/Platform.h" // Include for PLATFORM_WINDOWS etc. if needed for logging
#include "Logging/LogMacros.h" // Include for UE_LOG

// Define a log category if needed, replace 'LogMassAPI' with your desired category name
// DECLARE_LOG_CATEGORY_EXTERN(LogMassAPI, Log, All);
// DEFINE_LOG_CATEGORY(LogMassAPI);

//
// This file contains the implementation for the Blueprint Function Library.
// It includes complex "CustomThunk" functions that allow for dynamic pins in Blueprints.
//

//----------------------------------------------------------------------//
// Entity Operations
//----------------------------------------------------------------------//

bool UMassAPIBPFnLib::IsValid(const UObject* WorldContextObject, const FEntityHandle& EntityHandle)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        return MassAPI->IsValid(EntityHandle);
    }
    return false;
}

bool UMassAPIBPFnLib::HasFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        return MassAPI->HasFragment(EntityHandle, FragmentType);
    }
    return false;
}

bool UMassAPIBPFnLib::HasTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        return MassAPI->HasTag(EntityHandle, TagType);
    }
    return false;
}

// GET FRAGMENT
void UMassAPIBPFnLib::GetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess)
{
    checkNoEntry();
}

void UMassAPIBPFnLib::Generic_GetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, void* OutFragmentPtr, bool& bSuccess)
{
    bSuccess = false;
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !MassAPI->IsValid(EntityHandle) || !FragmentType || !OutFragmentPtr)
    {
        return;
    }

    if (!FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        // Optional: Log warning if type is invalid
        // UE_LOG(LogMassAPI, Warning, TEXT("GetFragment: Provided type '%s' is not a valid FMassFragment."), *FragmentType->GetName());
        return;
    }

    const FMassEntityManager& EntityManager = *MassAPI->GetEntityManager();
    FStructView FragmentView = EntityManager.GetFragmentDataStruct(EntityHandle, FragmentType);

    if (FragmentView.IsValid())
    {
        FragmentType->CopyScriptStruct(OutFragmentPtr, FragmentView.GetMemory());
        bSuccess = true;
    }
}

DEFINE_FUNCTION(UMassAPIBPFnLib::execGetFragment)
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
        FragmentType->InitializeStruct(OutFragmentPtr);
    }
    bSuccess = false;

    P_NATIVE_BEGIN
        Generic_GetFragment(WorldContextObject, EntityHandle, FragmentType, OutFragmentPtr, bSuccess);
    P_NATIVE_END
}


// SET FRAGMENT
void UMassAPIBPFnLib::SetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess)
{
    checkNoEntry();
}

void UMassAPIBPFnLib::Generic_SetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const void* InFragmentPtr, bool& bSuccess)
{
    bSuccess = false;
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !MassAPI->IsValid(EntityHandle) || !FragmentType || !InFragmentPtr)
    {
        return;
    }

    if (!FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        // UE_LOG(LogMassAPI, Warning, TEXT("SetFragment: Provided type '%s' is not a valid FMassFragment."), *FragmentType->GetName());
        return;
    }

    FMassEntityManager& EntityManager = *MassAPI->GetEntityManager();

    FInstancedStruct FragmentInstance;
    FragmentInstance.InitializeAs(FragmentType, static_cast<const uint8*>(InFragmentPtr));

    if (!FragmentInstance.IsValid())
    {
        // UE_LOG(LogMassAPI, Warning, TEXT("SetFragment: Failed to initialize FInstancedStruct for type '%s'."), *FragmentType->GetName());
        return;
    }

    if (MassAPI->HasFragment(EntityHandle, FragmentType))
    {
        EntityManager.SetEntityFragmentValues(EntityHandle, MakeArrayView(&FragmentInstance, 1));
        bSuccess = true;
    }
    else
    {
        MassAPI->AddFragment(EntityHandle, FragmentInstance);
        bSuccess = true;
    }
}

DEFINE_FUNCTION(UMassAPIBPFnLib::execSetFragment)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityHandle, EntityHandle);
    P_GET_OBJECT(UScriptStruct, FragmentType);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_GET_UBOOL_REF(bSuccess);
    P_FINISH;
    bSuccess = false;

    P_NATIVE_BEGIN
        Generic_SetFragment(WorldContextObject, EntityHandle, FragmentType, InFragmentPtr, bSuccess);
    P_NATIVE_END
}

// ADD TAG
void UMassAPIBPFnLib::AddTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !MassAPI->IsValid(EntityHandle) || !TagType)
    {
        return;
    }

    if (!TagType->IsChildOf(FMassTag::StaticStruct()))
    {
        // UE_LOG(LogMassAPI, Warning, TEXT("AddTag: Provided type '%s' is not a valid FMassTag."), *TagType->GetName());
        return;
    }

    // Call subsystem function - Note: FEntityHandle implicitly converts to FMassEntityHandle
    FMassEntityManager& EntityManager = *MassAPI->GetEntityManager();
    EntityManager.AddTagToEntity(EntityHandle, TagType);
}


void UMassAPIBPFnLib::DestroyEntity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        MassAPI->DestroyEntity(EntityHandle);
    }
}

//----------------------------------------------------------------------//
// Batch Entity Operations
//----------------------------------------------------------------------//

void UMassAPIBPFnLib::DestroyEntities(const UObject* WorldContextObject, const TArray<FEntityHandle>& EntityHandles)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || EntityHandles.Num() == 0)
    {
        return;
    }

    TArray<FMassEntityHandle> MassHandles;
    MassHandles.Reserve(EntityHandles.Num());
    for (const FEntityHandle& Handle : EntityHandles)
    {
        MassHandles.Add(Handle);
    }

    if (FMassEntityManager* EntityManager = MassAPI->GetEntityManager())
    {
        EntityManager->BatchDestroyEntities(MassHandles);
    }
}

// AddTagToEntities and RemoveTagFromEntities implementations removed as requested.

//----------------------------------------------------------------------//
// Shared Fragment Operations
//----------------------------------------------------------------------//

void UMassAPIBPFnLib::GetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess)
{
    checkNoEntry();
}

void UMassAPIBPFnLib::Generic_GetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, void* OutFragmentPtr, bool& bSuccess)
{
    bSuccess = false;
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !MassAPI->IsValid(EntityHandle) || !FragmentType || !OutFragmentPtr)
    {
        return;
    }

    if (!FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        // UE_LOG(LogMassAPI, Warning, TEXT("GetSharedFragment: Provided type '%s' is not a valid FMassSharedFragment."), *FragmentType->GetName());
        return;
    }

    const FMassEntityManager& EntityManager = *MassAPI->GetEntityManager();
    FConstStructView FragmentView = EntityManager.GetSharedFragmentDataStruct(EntityHandle, FragmentType);

    if (FragmentView.IsValid())
    {
        FragmentType->CopyScriptStruct(OutFragmentPtr, FragmentView.GetMemory());
        bSuccess = true;
    }
}

DEFINE_FUNCTION(UMassAPIBPFnLib::execGetSharedFragment)
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
        FragmentType->InitializeStruct(OutFragmentPtr);
    }
    bSuccess = false;

    P_NATIVE_BEGIN
        Generic_GetSharedFragment(WorldContextObject, EntityHandle, FragmentType, OutFragmentPtr, bSuccess);
    P_NATIVE_END
}

void UMassAPIBPFnLib::SetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess)
{
    checkNoEntry();
}

void UMassAPIBPFnLib::Generic_SetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const void* InFragmentPtr, bool& bSuccess)
{
    bSuccess = false;
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !MassAPI->IsValid(EntityHandle) || !FragmentType || !InFragmentPtr)
    {
        return;
    }

    if (!FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        // UE_LOG(LogMassAPI, Warning, TEXT("SetSharedFragment: Provided type '%s' is not a valid FMassSharedFragment."), *FragmentType->GetName());
        return;
    }

    FMassEntityManager& EntityManager = *MassAPI->GetEntityManager();
    const FSharedStruct& SharedStruct = EntityManager.GetOrCreateSharedFragment(*FragmentType, static_cast<const uint8*>(InFragmentPtr));
    bSuccess = EntityManager.AddSharedFragmentToEntity(EntityHandle, SharedStruct);
}

DEFINE_FUNCTION(UMassAPIBPFnLib::execSetSharedFragment)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityHandle, EntityHandle);
    P_GET_OBJECT(UScriptStruct, FragmentType);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_GET_UBOOL_REF(bSuccess);
    P_FINISH;
    bSuccess = false;

    P_NATIVE_BEGIN
        Generic_SetSharedFragment(WorldContextObject, EntityHandle, FragmentType, InFragmentPtr, bSuccess);
    P_NATIVE_END
}

//----------------------------------------------------------------------//
// Const Shared Fragment Operations
//----------------------------------------------------------------------//

void UMassAPIBPFnLib::GetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess)
{
    checkNoEntry();
}

void UMassAPIBPFnLib::Generic_GetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, void* OutFragmentPtr, bool& bSuccess)
{
    bSuccess = false;
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !MassAPI->IsValid(EntityHandle) || !FragmentType || !OutFragmentPtr)
    {
        return;
    }

    if (!FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        // UE_LOG(LogMassAPI, Warning, TEXT("GetConstSharedFragment: Provided type '%s' is not a valid FMassConstSharedFragment."), *FragmentType->GetName());
        return;
    }

    const FMassEntityManager& EntityManager = *MassAPI->GetEntityManager();
    FConstStructView FragmentView = EntityManager.GetConstSharedFragmentDataStruct(EntityHandle, FragmentType);

    if (FragmentView.IsValid())
    {
        FragmentType->CopyScriptStruct(OutFragmentPtr, FragmentView.GetMemory());
        bSuccess = true;
    }
}

DEFINE_FUNCTION(UMassAPIBPFnLib::execGetConstSharedFragment)
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
        FragmentType->InitializeStruct(OutFragmentPtr);
    }
    bSuccess = false;

    P_NATIVE_BEGIN
        Generic_GetConstSharedFragment(WorldContextObject, EntityHandle, FragmentType, OutFragmentPtr, bSuccess);
    P_NATIVE_END
}

void UMassAPIBPFnLib::SetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess)
{
    checkNoEntry();
}

void UMassAPIBPFnLib::Generic_SetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const void* InFragmentPtr, bool& bSuccess)
{
    bSuccess = false;
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !MassAPI->IsValid(EntityHandle) || !FragmentType || !InFragmentPtr)
    {
        return;
    }

    if (!FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        // UE_LOG(LogMassAPI, Warning, TEXT("SetConstSharedFragment: Provided type '%s' is not a valid FMassConstSharedFragment."), *FragmentType->GetName());
        return;
    }

    FMassEntityManager& EntityManager = *MassAPI->GetEntityManager();
    const FConstSharedStruct& ConstSharedStruct = EntityManager.GetOrCreateConstSharedFragment(*FragmentType, static_cast<const uint8*>(InFragmentPtr));
    bSuccess = EntityManager.AddConstSharedFragmentToEntity(EntityHandle, ConstSharedStruct);
}

DEFINE_FUNCTION(UMassAPIBPFnLib::execSetConstSharedFragment)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityHandle, EntityHandle);
    P_GET_OBJECT(UScriptStruct, FragmentType);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_GET_UBOOL_REF(bSuccess);
    P_FINISH;
    bSuccess = false;

    P_NATIVE_BEGIN
        Generic_SetConstSharedFragment(WorldContextObject, EntityHandle, FragmentType, InFragmentPtr, bSuccess);
    P_NATIVE_END
}


//----------------------------------------------------------------------//
// Entity Building
//----------------------------------------------------------------------//

FEntityHandle UMassAPIBPFnLib::BuildEntityFromTemplateData(const UObject* WorldContextObject, const FEntityTemplateData& TemplateData)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        if (FMassEntityTemplateData* Data = TemplateData.Get())
        {
            TArray<FMassEntityHandle> MassHandles = MassAPI->BuildEntities(1, *Data);
            if (MassHandles.Num() > 0)
            {
                return FEntityHandle(MassHandles[0]);
            }
        }
    }
    return FEntityHandle();
}

TArray<FEntityHandle> UMassAPIBPFnLib::BuildEntitiesFromTemplateData(const UObject* WorldContextObject, int32 Quantity, const FEntityTemplateData& TemplateData)
{
    TArray<FEntityHandle> BPHandles;
    if (Quantity <= 0)
    {
        return BPHandles;
    }

    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        if (FMassEntityTemplateData* Data = TemplateData.Get())
        {
            TArray<FMassEntityHandle> MassHandles = MassAPI->BuildEntities(Quantity, *Data);
            BPHandles.Reserve(MassHandles.Num());
            for (const FMassEntityHandle& MassHandle : MassHandles)
            {
                BPHandles.Add(FEntityHandle(MassHandle));
            }
        }
    }
    return BPHandles;
}


//----------------------------------------------------------------------//
// TemplateData Operations
//----------------------------------------------------------------------//

FEntityTemplateData UMassAPIBPFnLib::GetTemplateData(const UObject* WorldContextObject, const FEntityTemplate& Template)
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

bool UMassAPIBPFnLib::IsEmpty_TemplateData(const FEntityTemplateData& TemplateData)
{
    if (const FMassEntityTemplateData* Data = TemplateData.Get())
    {
        return Data->IsEmpty();
    }
    return true;
}

bool UMassAPIBPFnLib::HasFragment_TemplateData(const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    if (const FMassEntityTemplateData* Data = TemplateData.Get())
    {
        if (FragmentType && FragmentType->IsChildOf(FMassFragment::StaticStruct()))
        {
            return Data->GetCompositionDescriptor().Fragments.Contains(*FragmentType);
        }
    }
    return false;
}

void UMassAPIBPFnLib::AddTag_TemplateData(FEntityTemplateData& TemplateData, UScriptStruct* TagType)
{
    if (FMassEntityTemplateData* Data = TemplateData.Get())
    {
        if (TagType && TagType->IsChildOf(FMassTag::StaticStruct()))
        {
            Data->AddTag(*TagType);
        }
    }
}

void UMassAPIBPFnLib::RemoveTag_TemplateData(FEntityTemplateData& TemplateData, UScriptStruct* TagType)
{
    if (FMassEntityTemplateData* Data = TemplateData.Get())
    {
        if (TagType && TagType->IsChildOf(FMassTag::StaticStruct()))
        {
            Data->RemoveTag(*TagType);
        }
    }
}

bool UMassAPIBPFnLib::HasTag_TemplateData(const FEntityTemplateData& TemplateData, UScriptStruct* TagType)
{
    if (const FMassEntityTemplateData* Data = TemplateData.Get())
    {
        if (TagType && TagType->IsChildOf(FMassTag::StaticStruct()))
        {
            return Data->GetCompositionDescriptor().Tags.Contains(*TagType);
        }
    }
    return false;
}

// GetFragmentFromTemplate
void UMassAPIBPFnLib::GetFragmentFromTemplate(const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess)
{
    checkNoEntry();
}

void UMassAPIBPFnLib::Generic_GetFragmentFromTemplate(const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, void* OutFragmentPtr, bool& bSuccess)
{
    bSuccess = false;
    if (!FragmentType || !OutFragmentPtr)
    {
        return;
    }

    if (const FMassEntityTemplateData* Data = TemplateData.Get())
    {
        TConstArrayView<FInstancedStruct> InitialValues = Data->GetInitialFragmentValues();
        for (const FInstancedStruct& Value : InitialValues)
        {
            if (Value.GetScriptStruct() == FragmentType)
            {
                FragmentType->CopyScriptStruct(OutFragmentPtr, Value.GetMemory());
                bSuccess = true;
                return;
            }
        }
    }
}

DEFINE_FUNCTION(UMassAPIBPFnLib::execGetFragmentFromTemplate)
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
        Generic_GetFragmentFromTemplate(TemplateData, FragmentType, OutFragmentPtr, bSuccess);
    P_NATIVE_END
}

// SetFragmentInTemplate - Reverted to copying implementation
void UMassAPIBPFnLib::SetFragmentInTemplate(FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const FGenericStruct& InFragment)
{
    checkNoEntry();
}

void UMassAPIBPFnLib::Generic_SetFragmentInTemplate(FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const void* InFragmentPtr)
{
    if (!FragmentType || !InFragmentPtr || !FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        return;
    }

    const FMassEntityTemplateData* OldData = TemplateData.Get();
    TSharedPtr<FMassEntityTemplateData> NewData = MakeShared<FMassEntityTemplateData>();

    if (OldData)
    {
        // 1. Copy everything from OldData to NewData, EXCEPT for the initial value of the fragment we are setting.
        NewData->SetTemplateName(OldData->GetTemplateName());

        // Copy fragment types
        OldData->GetCompositionDescriptor().Fragments.ExportTypes([&](const UScriptStruct* Type)
            {
                NewData->AddFragment(*Type);
                return true;
            });

        // Copy initial values, skipping the one to be replaced.
        for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues())
        {
            if (Fragment.GetScriptStruct() != FragmentType)
            {
                NewData->AddFragment(FConstStructView(Fragment));
            }
        }

        // Copy tag types
        OldData->GetTags().ExportTypes([&](const UScriptStruct* TagType)
            {
                NewData->AddTag(*TagType);
                return true;
            });

        // Copy Shared and Const Shared Fragment Handles
        const FMassArchetypeSharedFragmentValues& SharedValues = OldData->GetSharedFragmentValues();
        for (const FSharedStruct& SharedFragment : SharedValues.GetSharedFragments()) { NewData->AddSharedFragment(SharedFragment); }
        for (const FConstSharedStruct& ConstSharedFragment : SharedValues.GetConstSharedFragments()) { NewData->AddConstSharedFragment(ConstSharedFragment); }

        // Copy Chunk Fragment types (Assuming FMassEntityTemplateData provides a suitable method)
        OldData->GetCompositionDescriptor().ChunkFragments.ExportTypes([&](const UScriptStruct* Type)
            {
                // if (NewData && Type) NewData->AddChunkFragment(*Type); // Need appropriate API in FMassEntityTemplateData
                return true;
            });

        // Archetype Creation Params - REMOVED due to const correctness issue
        // NewData->GetArchetypeCreationParams() = OldData->GetArchetypeCreationParams();

        // Copy Object Initializers (Requires getter/setter in FMassEntityTemplateData)
        // NewData->GetMutableObjectInitializers() = OldData->GetObjectFragmentInitializers();
    }

    // 2. Now, add the new/updated fragment value using the method that stores initial values.
    NewData->AddFragment(FConstStructView(FragmentType, static_cast<const uint8*>(InFragmentPtr)));

    // 3. Replace the TSharedPtr in the handle.
    TemplateData = FEntityTemplateData(NewData);
}


DEFINE_FUNCTION(UMassAPIBPFnLib::execSetFragmentInTemplate)
{
    P_GET_STRUCT_REF(FEntityTemplateData, TemplateData);
    P_GET_OBJECT(UScriptStruct, FragmentType);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_FINISH;

    P_NATIVE_BEGIN
        Generic_SetFragmentInTemplate(TemplateData, FragmentType, InFragmentPtr);
    P_NATIVE_END
}

// SetSharedFragmentInTemplate - Corrected copying implementation
void UMassAPIBPFnLib::SetSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const FGenericStruct& InFragment)
{
    checkNoEntry();
}

void UMassAPIBPFnLib::Generic_SetSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const void* InFragmentPtr)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !FragmentType || !InFragmentPtr || !FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        return;
    }

    const FMassEntityTemplateData* OldData = TemplateData.Get();
    TSharedPtr<FMassEntityTemplateData> NewData = MakeShared<FMassEntityTemplateData>();

    FMassEntityManager& EntityManager = *MassAPI->GetEntityManager();
    // Get the handle for the NEW shared fragment data
    const FSharedStruct& NewSharedStructHandle = EntityManager.GetOrCreateSharedFragment(*FragmentType, static_cast<const uint8*>(InFragmentPtr));

    if (OldData)
    {
        // Copy everything except the specific shared fragment handle we are replacing
        NewData->SetTemplateName(OldData->GetTemplateName());

        OldData->GetCompositionDescriptor().Fragments.ExportTypes([&](const UScriptStruct* Type) { NewData->AddFragment(*Type); return true; });
        for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues()) { NewData->AddFragment(FConstStructView(Fragment)); }
        OldData->GetTags().ExportTypes([&](const UScriptStruct* TagType) { NewData->AddTag(*TagType); return true; });

        const FMassArchetypeSharedFragmentValues& SharedValues = OldData->GetSharedFragmentValues();
        // Copy existing Shared Fragments, SKIPPING the one with the same type as the one we are setting
        for (const FSharedStruct& SharedFragment : SharedValues.GetSharedFragments())
        {
            if (SharedFragment.GetScriptStruct() != FragmentType)
            {
                NewData->AddSharedFragment(SharedFragment);
            }
        }
        // Copy ALL Const Shared Fragments
        for (const FConstSharedStruct& ConstSharedFragment : SharedValues.GetConstSharedFragments()) { NewData->AddConstSharedFragment(ConstSharedFragment); }

        // Copy Chunk Fragments
        OldData->GetCompositionDescriptor().ChunkFragments.ExportTypes([&](const UScriptStruct* Type)
            {
                // if(NewData && Type) NewData->AddChunkFragment(*Type); // Assuming AddChunkFragment exists
                return true;
            });

        // Creation Params - REMOVED due to const issue
        // NewData->GetArchetypeCreationParams() = OldData->GetArchetypeCreationParams();
        // Copy Object Initializers if needed
    }

    // Add the NEW Shared Fragment handle (this adds the type to composition if needed)
    NewData->AddSharedFragment(NewSharedStructHandle);

    // Replace the handle's internal pointer
    TemplateData = FEntityTemplateData(NewData);
}

DEFINE_FUNCTION(UMassAPIBPFnLib::execSetSharedFragmentInTemplate)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityTemplateData, TemplateData);
    P_GET_OBJECT(UScriptStruct, FragmentType);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_FINISH;

    P_NATIVE_BEGIN
        Generic_SetSharedFragmentInTemplate(WorldContextObject, TemplateData, FragmentType, InFragmentPtr);
    P_NATIVE_END
}

// SetConstSharedFragmentInTemplate - Corrected copying implementation
void UMassAPIBPFnLib::SetConstSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const FGenericStruct& InFragment)
{
    checkNoEntry();
}

void UMassAPIBPFnLib::Generic_SetConstSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const void* InFragmentPtr)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !FragmentType || !InFragmentPtr || !FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        return;
    }

    const FMassEntityTemplateData* OldData = TemplateData.Get();
    TSharedPtr<FMassEntityTemplateData> NewData = MakeShared<FMassEntityTemplateData>();

    FMassEntityManager& EntityManager = *MassAPI->GetEntityManager();
    // Get the handle for the NEW const shared fragment data
    const FConstSharedStruct& NewConstSharedStructHandle = EntityManager.GetOrCreateConstSharedFragment(*FragmentType, static_cast<const uint8*>(InFragmentPtr));

    if (OldData)
    {
        // Copy everything except the specific const shared fragment handle we are replacing
        NewData->SetTemplateName(OldData->GetTemplateName());

        OldData->GetCompositionDescriptor().Fragments.ExportTypes([&](const UScriptStruct* Type) { NewData->AddFragment(*Type); return true; });
        for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues()) { NewData->AddFragment(FConstStructView(Fragment)); }
        OldData->GetTags().ExportTypes([&](const UScriptStruct* TagType) { NewData->AddTag(*TagType); return true; });

        const FMassArchetypeSharedFragmentValues& SharedValues = OldData->GetSharedFragmentValues();
        // Copy ALL Shared Fragments
        for (const FSharedStruct& SharedFragment : SharedValues.GetSharedFragments()) { NewData->AddSharedFragment(SharedFragment); }
        // Copy existing Const Shared Fragments, SKIPPING the one with the same type
        for (const FConstSharedStruct& ConstSharedFragment : SharedValues.GetConstSharedFragments())
        {
            if (ConstSharedFragment.GetScriptStruct() != FragmentType)
            {
                NewData->AddConstSharedFragment(ConstSharedFragment);
            }
        }
        // Copy Chunk Fragments
        OldData->GetCompositionDescriptor().ChunkFragments.ExportTypes([&](const UScriptStruct* Type)
            {
                // if(NewData && Type) NewData->AddChunkFragment(*Type); // Assuming AddChunkFragment exists
                return true;
            });

        // Creation Params - REMOVED due to const issue
        // NewData->GetArchetypeCreationParams() = OldData->GetArchetypeCreationParams();
        // Copy Object Initializers if needed
    }

    // Add the NEW Const Shared Fragment handle
    NewData->AddConstSharedFragment(NewConstSharedStructHandle);

    // Replace the handle's internal pointer
    TemplateData = FEntityTemplateData(NewData);
}

DEFINE_FUNCTION(UMassAPIBPFnLib::execSetConstSharedFragmentInTemplate)
{
    P_GET_OBJECT(UObject, WorldContextObject);
    P_GET_STRUCT_REF(FEntityTemplateData, TemplateData);
    P_GET_OBJECT(UScriptStruct, FragmentType);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    const void* InFragmentPtr = Stack.MostRecentPropertyAddress;
    P_FINISH;

    P_NATIVE_BEGIN
        Generic_SetConstSharedFragmentInTemplate(WorldContextObject, TemplateData, FragmentType, InFragmentPtr);
    P_NATIVE_END
}

//----------------------------------------------------------------------//
// Entity Querying & BP Processors
//----------------------------------------------------------------------//

bool UMassAPIBPFnLib::MatchEntityQuery(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, const FEntityQuery& Query)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        return MassAPI->MatchQuery(EntityHandle, Query);
    }
    return false;
}

// Corrected GetMatchingEntities
TArray<FEntityHandle> UMassAPIBPFnLib::GetMatchingEntities(const UObject* WorldContextObject, const FEntityQuery& Query)
{
    TArray<FEntityHandle> BPHandles;
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI)
    {
        return BPHandles;
    }

    FMassEntityManager* EntityManager = MassAPI->GetEntityManager();
    if (!EntityManager)
    {
        return BPHandles;
    }

    // 1. Create the native FMassEntityQuery, passing the EntityManager reference
    FMassEntityQuery NativeQuery(EntityManager->AsShared());

    // 2. Iterate through the BP Query lists and add individual requirements using the correct functions
    // Using ReadOnly access as we're just fetching handles.
    for (const TObjectPtr<UScriptStruct>& Struct : Query.AllList)
    {
        if (!Struct) continue;

        if (Struct->IsChildOf(FMassTag::StaticStruct()))
        {
            NativeQuery.AddTagRequirement(*Struct, EMassFragmentPresence::All);
        }
        else if (Struct->IsChildOf(FMassSharedFragment::StaticStruct()))
        {
            NativeQuery.AddSharedRequirement(Struct, EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
        }
        else if (Struct->IsChildOf(FMassConstSharedFragment::StaticStruct()))
        {
            NativeQuery.AddConstSharedRequirement(Struct, EMassFragmentPresence::All);
        }
        else if (Struct->IsChildOf(FMassChunkFragment::StaticStruct()))
        {
            NativeQuery.AddChunkRequirement(Struct, EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
        }
        else if (Struct->IsChildOf(FMassFragment::StaticStruct()))
        {
            NativeQuery.AddRequirement(Struct, EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
        }
        else
        {
            // UE_LOG(LogMassAPI, Warning, TEXT("GetMatchingEntities: Skipping invalid type '%s' in AllList."), *Struct->GetName());
        }
    }
    for (const TObjectPtr<UScriptStruct>& Struct : Query.AnyList)
    {
        if (!Struct) continue;

        if (Struct->IsChildOf(FMassTag::StaticStruct()))
        {
            NativeQuery.AddTagRequirement(*Struct, EMassFragmentPresence::Any);
        }
        // NOTE: FMassEntityQuery does not support 'Any' presence for Shared, ConstShared, or Chunk fragments.
        // FMassFragmentRequirements::AddSharedRequirement etc. check() for this.
        // We will only add Fragments to the 'Any' list.
        else if (Struct->IsChildOf(FMassFragment::StaticStruct()))
        {
            NativeQuery.AddRequirement(Struct, EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Any);
        }
        else
        {
            // UE_LOG(LogMassAPI, Warning, TEXT("GetMatchingEntities: Skipping unsupported type '%s' in AnyList. Only Fragments and Tags are supported."), *Struct->GetName());
        }
    }
    for (const TObjectPtr<UScriptStruct>& Struct : Query.NoneList)
    {
        if (!Struct) continue;

        if (Struct->IsChildOf(FMassTag::StaticStruct()))
        {
            NativeQuery.AddTagRequirement(*Struct, EMassFragmentPresence::None);
        }
        else if (Struct->IsChildOf(FMassSharedFragment::StaticStruct()))
        {
            NativeQuery.AddSharedRequirement(Struct, EMassFragmentAccess::None, EMassFragmentPresence::None); // Access None needed for None presence
        }
        else if (Struct->IsChildOf(FMassConstSharedFragment::StaticStruct()))
        {
            NativeQuery.AddConstSharedRequirement(Struct, EMassFragmentPresence::None);
        }
        else if (Struct->IsChildOf(FMassChunkFragment::StaticStruct()))
        {
            NativeQuery.AddChunkRequirement(Struct, EMassFragmentAccess::None, EMassFragmentPresence::None); // Access None needed for None presence
        }
        else if (Struct->IsChildOf(FMassFragment::StaticStruct()))
        {
            NativeQuery.AddRequirement(Struct, EMassFragmentAccess::None, EMassFragmentPresence::None); // Access None needed for None presence
        }
        else
        {
            // UE_LOG(LogMassAPI, Warning, TEXT("GetMatchingEntities: Skipping invalid type '%s' in NoneList."), *Struct->GetName());
        }
    }

    // 3. REMOVED Validity Check Block - Relying on GetMatchingEntityHandles calling CacheArchetypes internally

    // 4. Execute the query and get the results (UE 5.6+ returns the array)
    // IMPORTANT: GetMatchingEntityHandles calls CacheArchetypes internally in 5.6
    TArray<FMassEntityHandle> NativeHandles = NativeQuery.GetMatchingEntityHandles();

    // 5. Convert the results to the Blueprint-safe FEntityHandle
    BPHandles.Reserve(NativeHandles.Num());
    for (const FMassEntityHandle& NativeHandle : NativeHandles)
    {
        BPHandles.Add(FEntityHandle(NativeHandle)); // Using the FEntityHandle constructor
    }

    // 6. Return the BP-safe array
    return BPHandles;
}

