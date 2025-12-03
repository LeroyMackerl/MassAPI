/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#include "MassAPIBPFnLib.h"
#include "MassAPISubsystem.h"
#include "MassAPIEnums.h" // (新) 包含新的枚举头文件
#include "MassEntityView.h"
#include "MassEntitySubsystem.h"
#include "UObject/StructOnScope.h"
#include "MassEntityQuery.h" // Correct include
#include "MassRequirements.h" // Include for EMassFragmentPresence, FMassFragmentRequirements methods
#include "MassExternalSubsystemTraits.h" // Include for query subsystem requirements if needed
#include "MassDebugger.h" // Include for logging query validity if needed
#include "HAL/Platform.h" // Include for PLATFORM_WINDOWS etc. if needed for logging
#include "Runtime/Launch/Resources/Version.h"
#include "Logging/LogMacros.h" // Include for UE_LOG

// (新) 包含 MassAPIStructs.h 以识别重命名的 FEntityFlagFragment
#include "MassAPIStructs.h" 

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
    // > UE 5.7 accessor methods
    #define GET_FRAGMENTS GetFragments()
    #define GET_TAGS GetTags()
    #define GET_CHUNK_FRAGMENTS GetChunkFragments()
#else
    // < UE 5.7 accessor methods
    #define GET_FRAGMENTS Fragments
    #define GET_TAGS Tags
    #define GET_CHUNK_FRAGMENTS ChunkFragments
#endif

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

// REMOVE FRAGMENT (Entity Operation)
bool UMassAPIBPFnLib::RemoveFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !FragmentType || !FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        return false;
    }

    // Use the generic non-templated function added to the subsystem
    return MassAPI->RemoveFragment(EntityHandle, FragmentType);
}

// REMOVE SHARED FRAGMENT (Entity Operation)
bool UMassAPIBPFnLib::RemoveSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !FragmentType || !FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        return false;
    }

    // Use the generic non-templated function added to the subsystem
    return MassAPI->RemoveSharedFragment(EntityHandle, FragmentType);
}

// REMOVE CONST SHARED FRAGMENT (Entity Operation)
bool UMassAPIBPFnLib::RemoveConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !FragmentType || !FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        return false;
    }

    // Use the generic non-templated function added to the subsystem
    return MassAPI->RemoveConstSharedFragment(EntityHandle, FragmentType);
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

void UMassAPIBPFnLib::RemoveTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !MassAPI->IsValid(EntityHandle) || !TagType)
    {
        return;
    }

    if (!TagType->IsChildOf(FMassTag::StaticStruct()))
    {
        // UE_LOG(LogMassAPI, Warning, TEXT("RemoveTag: Provided type '%s' is not a valid FMassTag."), *TagType->GetName());
        return;
    }

    // Call subsystem function
    FMassEntityManager& EntityManager = *MassAPI->GetEntityManager();
    EntityManager.RemoveTagFromEntity(EntityHandle, TagType);
}

void UMassAPIBPFnLib::DestroyEntity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        MassAPI->Defer().DestroyEntity(EntityHandle);
        // MassAPI->DestroyEntity(EntityHandle);
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
            Data->Sort();
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

FEntityTemplateData UMassAPIBPFnLib::Conv_TemplateToTemplateData(const UObject* WorldContextObject, const FEntityTemplate& Template)
{
    // This is the auto-cast conversion function
    // It simply calls the existing GetTemplateData function
    return GetTemplateData(WorldContextObject, Template);
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
            return Data->GetCompositionDescriptor().GET_FRAGMENTS.Contains(*FragmentType);
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
            return Data->GetCompositionDescriptor().GET_TAGS.Contains(*TagType);
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
        OldData->GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type)
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
        OldData->GetCompositionDescriptor().GET_TAGS.ExportTypes([&](const UScriptStruct* TagType)
            {
                NewData->AddTag(*TagType);
                return true;
            });

        // Copy Shared and Const Shared Fragment Handles
        const FMassArchetypeSharedFragmentValues& SharedValues = OldData->GetSharedFragmentValues();
        for (const FSharedStruct& SharedFragment : SharedValues.GetSharedFragments()) { NewData->AddSharedFragment(SharedFragment); }
        for (const FConstSharedStruct& ConstSharedFragment : SharedValues.GetConstSharedFragments()) { NewData->AddConstSharedFragment(ConstSharedFragment); }

        // Copy Chunk Fragment types (Assuming FMassEntityTemplateData provides a suitable method)
        OldData->GetCompositionDescriptor().GET_CHUNK_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type)
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

// REMOVE FRAGMENT IN TEMPLATE
void UMassAPIBPFnLib::RemoveFragmentInTemplate(FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    Generic_RemoveFragmentInTemplate(TemplateData, FragmentType);
}

void UMassAPIBPFnLib::Generic_RemoveFragmentInTemplate(FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    if (!FragmentType || !FragmentType->IsChildOf(FMassFragment::StaticStruct()))
    {
        return;
    }

    const FMassEntityTemplateData* OldData = TemplateData.Get();
    TSharedPtr<FMassEntityTemplateData> NewData = MakeShared<FMassEntityTemplateData>();

    if (OldData)
    {
        // 1. Copy everything from OldData to NewData
        NewData->SetTemplateName(OldData->GetTemplateName());

        // Copy fragment types and initial values, SKIPPING the one to be removed.
        OldData->GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type)
            {
                if (Type != FragmentType)
                {
                    NewData->AddFragment(*Type);
                }
                return true;
            });

        // Copy initial values, skipping the one to be removed.
        for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues())
        {
            if (Fragment.GetScriptStruct() != FragmentType)
            {
                NewData->AddFragment(FConstStructView(Fragment));
            }
        }

        // Copy other types (Tags, Shared, ConstShared, Chunk)
        OldData->GetCompositionDescriptor().GET_TAGS.ExportTypes([&](const UScriptStruct* TagType) { NewData->AddTag(*TagType); return true; });
        const FMassArchetypeSharedFragmentValues& SharedValues = OldData->GetSharedFragmentValues();
        for (const FSharedStruct& SharedFragment : SharedValues.GetSharedFragments()) { NewData->AddSharedFragment(SharedFragment); }
        for (const FConstSharedStruct& ConstSharedFragment : SharedValues.GetConstSharedFragments()) { NewData->AddConstSharedFragment(ConstSharedFragment); }
        OldData->GetCompositionDescriptor().GET_CHUNK_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) { return true; }); // Placeholder
    }

    // 2. Replace the TSharedPtr in the handle.
    TemplateData = FEntityTemplateData(NewData);
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

        OldData->GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) { NewData->AddFragment(*Type); return true; });
        for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues()) { NewData->AddFragment(FConstStructView(Fragment)); }
        OldData->GetCompositionDescriptor().GET_TAGS.ExportTypes([&](const UScriptStruct* TagType) { NewData->AddTag(*TagType); return true; });

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
        OldData->GetCompositionDescriptor().GET_CHUNK_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type)
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

// REMOVE SHARED FRAGMENT IN TEMPLATE
void UMassAPIBPFnLib::RemoveSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    Generic_RemoveSharedFragmentInTemplate(WorldContextObject, TemplateData, FragmentType);
}

void UMassAPIBPFnLib::Generic_RemoveSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !FragmentType || !FragmentType->IsChildOf(FMassSharedFragment::StaticStruct()))
    {
        return;
    }

    const FMassEntityTemplateData* OldData = TemplateData.Get();
    TSharedPtr<FMassEntityTemplateData> NewData = MakeShared<FMassEntityTemplateData>();

    if (OldData)
    {
        // 1. Copy everything from OldData to NewData
        NewData->SetTemplateName(OldData->GetTemplateName());

        // Copy Fragments and Tags (no change)
        OldData->GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) { NewData->AddFragment(*Type); return true; });
        for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues()) { NewData->AddFragment(FConstStructView(Fragment)); }
        OldData->GetCompositionDescriptor().GET_TAGS.ExportTypes([&](const UScriptStruct* TagType) { NewData->AddTag(*TagType); return true; });
        OldData->GetCompositionDescriptor().GET_CHUNK_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) { return true; }); // Placeholder

        const FMassArchetypeSharedFragmentValues& SharedValues = OldData->GetSharedFragmentValues();
        // Copy existing Shared Fragments, SKIPPING the one with the same type as the one we are removing
        for (const FSharedStruct& SharedFragment : SharedValues.GetSharedFragments())
        {
            if (SharedFragment.GetScriptStruct() != FragmentType)
            {
                NewData->AddSharedFragment(SharedFragment);
            }
        }
        // Copy ALL Const Shared Fragments (no change)
        for (const FConstSharedStruct& ConstSharedFragment : SharedValues.GetConstSharedFragments()) { NewData->AddConstSharedFragment(ConstSharedFragment); }
    }

    // 2. Replace the TSharedPtr in the handle.
    TemplateData = FEntityTemplateData(NewData);
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

        OldData->GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) { NewData->AddFragment(*Type); return true; });
        for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues()) { NewData->AddFragment(FConstStructView(Fragment)); }
        OldData->GetCompositionDescriptor().GET_TAGS.ExportTypes([&](const UScriptStruct* TagType) { NewData->AddTag(*TagType); return true; });

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
        OldData->GetCompositionDescriptor().GET_CHUNK_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type)
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

// REMOVE CONST SHARED FRAGMENT IN TEMPLATE
void UMassAPIBPFnLib::RemoveConstSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    Generic_RemoveConstSharedFragmentInTemplate(WorldContextObject, TemplateData, FragmentType);
}

void UMassAPIBPFnLib::Generic_RemoveConstSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI || !FragmentType || !FragmentType->IsChildOf(FMassConstSharedFragment::StaticStruct()))
    {
        return;
    }

    const FMassEntityTemplateData* OldData = TemplateData.Get();
    TSharedPtr<FMassEntityTemplateData> NewData = MakeShared<FMassEntityTemplateData>();

    if (OldData)
    {
        // 1. Copy everything from OldData to NewData
        NewData->SetTemplateName(OldData->GetTemplateName());

        // Copy Fragments and Tags (no change)
        OldData->GetCompositionDescriptor().GET_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) { NewData->AddFragment(*Type); return true; });
        for (const FInstancedStruct& Fragment : OldData->GetInitialFragmentValues()) { NewData->AddFragment(FConstStructView(Fragment)); }
        OldData->GetCompositionDescriptor().GET_TAGS.ExportTypes([&](const UScriptStruct* TagType) { NewData->AddTag(*TagType); return true; });
        OldData->GetCompositionDescriptor().GET_CHUNK_FRAGMENTS.ExportTypes([&](const UScriptStruct* Type) { return true; }); // Placeholder

        const FMassArchetypeSharedFragmentValues& SharedValues = OldData->GetSharedFragmentValues();
        // Copy ALL Shared Fragments (no change)
        for (const FSharedStruct& SharedFragment : SharedValues.GetSharedFragments()) { NewData->AddSharedFragment(SharedFragment); }
        // Copy existing Const Shared Fragments, SKIPPING the one with the same type as the one we are removing
        for (const FConstSharedStruct& ConstSharedFragment : SharedValues.GetConstSharedFragments())
        {
            if (ConstSharedFragment.GetScriptStruct() != FragmentType)
            {
                NewData->AddConstSharedFragment(ConstSharedFragment);
            }
        }
    }

    // 2. Replace the TSharedPtr in the handle.
    TemplateData = FEntityTemplateData(NewData);
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

// (!!! 已修改为两阶段过滤: Composition + Flags !!!)
TArray<FEntityHandle> UMassAPIBPFnLib::GetMatchingEntities(const UObject* WorldContextObject, UPARAM(ref) const FEntityQuery& Query)
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

    // 1. 创建原生的 FMassEntityQuery，如之前一样
    FMassEntityQuery NativeQuery(EntityManager->AsShared());

    // 2. 遍历 BP Query 列表并添加 Composition 要求
    for (const TObjectPtr<UScriptStruct>& Struct : Query.AllList)
    {
        if (!Struct) continue;
        if (Struct->IsChildOf(FMassTag::StaticStruct())) { NativeQuery.AddTagRequirement(*Struct, EMassFragmentPresence::All); }
        else if (Struct->IsChildOf(FMassSharedFragment::StaticStruct())) { NativeQuery.AddSharedRequirement(Struct, EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All); }
        else if (Struct->IsChildOf(FMassConstSharedFragment::StaticStruct())) { NativeQuery.AddConstSharedRequirement(Struct, EMassFragmentPresence::All); }
        else if (Struct->IsChildOf(FMassChunkFragment::StaticStruct())) { NativeQuery.AddChunkRequirement(Struct, EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All); }
        else if (Struct->IsChildOf(FMassFragment::StaticStruct())) { NativeQuery.AddRequirement(Struct, EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All); }
    }
    for (const TObjectPtr<UScriptStruct>& Struct : Query.AnyList)
    {
        if (!Struct) continue;
        if (Struct->IsChildOf(FMassTag::StaticStruct())) { NativeQuery.AddTagRequirement(*Struct, EMassFragmentPresence::Any); }
        else if (Struct->IsChildOf(FMassFragment::StaticStruct())) { NativeQuery.AddRequirement(Struct, EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Any); }
    }
    for (const TObjectPtr<UScriptStruct>& Struct : Query.NoneList)
    {
        if (!Struct) continue;
        if (Struct->IsChildOf(FMassTag::StaticStruct())) { NativeQuery.AddTagRequirement(*Struct, EMassFragmentPresence::None); }
        else if (Struct->IsChildOf(FMassSharedFragment::StaticStruct())) { NativeQuery.AddSharedRequirement(Struct, EMassFragmentAccess::None, EMassFragmentPresence::None); }
        else if (Struct->IsChildOf(FMassConstSharedFragment::StaticStruct())) { NativeQuery.AddConstSharedRequirement(Struct, EMassFragmentPresence::None); }
        else if (Struct->IsChildOf(FMassChunkFragment::StaticStruct())) { NativeQuery.AddChunkRequirement(Struct, EMassFragmentAccess::None, EMassFragmentPresence::None); }
        else if (Struct->IsChildOf(FMassFragment::StaticStruct())) { NativeQuery.AddRequirement(Struct, EMassFragmentAccess::None, EMassFragmentPresence::None); }
    }

    // 3. 执行原生查询，获取所有匹配 *Composition* 的实体
    TArray<FMassEntityHandle> CompositionMatches = NativeQuery.GetMatchingEntityHandles();

    // 4. 从查询中获取缓存的标志位掩码
    const int64 AllFlagsQuery = Query.GetAllFlagsBitmask();
    const int64 AnyFlagsQuery = Query.GetAnyFlagsBitmask();
    const int64 NoneFlagsQuery = Query.GetNoneFlagsBitmask();

    // 5. 如果没有标志位查询，我们可以直接返回结果
    if (AllFlagsQuery == 0 && AnyFlagsQuery == 0 && NoneFlagsQuery == 0)
    {
        BPHandles.Reserve(CompositionMatches.Num());
        for (const FMassEntityHandle& NativeHandle : CompositionMatches)
        {
            BPHandles.Add(FEntityHandle(NativeHandle));
        }
        return BPHandles;
    }

    // 6. 执行第二遍过滤：检查我们的自定义标志位
    BPHandles.Reserve(CompositionMatches.Num()); // 优化：预分配最坏情况的大小
    for (const FMassEntityHandle& Handle : CompositionMatches)
    {
        // (已重命名: FMassEntityFlagFragment -> FEntityFlagFragment)
        const FEntityFlagFragment* FlagFragment = EntityManager->GetFragmentDataPtr<FEntityFlagFragment>(Handle);
        const int64 EntityFlags = FlagFragment ? FlagFragment->Flags : 0;

        // 执行检查
        const bool bAllFlags = (AllFlagsQuery == 0) || ((EntityFlags & AllFlagsQuery) == AllFlagsQuery);
        const bool bAnyFlags = (AnyFlagsQuery == 0) || ((EntityFlags & AnyFlagsQuery) != 0);
        const bool bNoneFlags = (NoneFlagsQuery == 0) || ((EntityFlags & NoneFlagsQuery) == 0);

        if (bAllFlags && bAnyFlags && bNoneFlags)
        {
            // 只有通过了标志位检查的才被添加到最终数组
            BPHandles.Add(FEntityHandle(Handle));
        }
    }

    // 7. 返回 BP 安全的数组
    return BPHandles;
}


//----------------------------------------------------------------------//
// (新) Flag Fragment Operations (TemplateData)
//----------------------------------------------------------------------//

int64 UMassAPIBPFnLib::GetTemplateFlags(const FEntityTemplateData& TemplateData)
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

bool UMassAPIBPFnLib::HasTemplateFlag(const FEntityTemplateData& TemplateData, EEntityFlags FlagToTest)
{
    if (FlagToTest >= EEntityFlags::EEntityFlags_MAX)
    {
        return false;
    }

    const int64 TemplateFlags = GetTemplateFlags(TemplateData);
    return (TemplateFlags & (1LL << static_cast<uint8>(FlagToTest))) != 0;
}

void UMassAPIBPFnLib::SetTemplateFlag(FEntityTemplateData& TemplateData, EEntityFlags FlagToSet)
{
    if (FlagToSet >= EEntityFlags::EEntityFlags_MAX)
    {
        return;
    }

    // 1. Get current flags
    const int64 CurrentFlags = GetTemplateFlags(TemplateData);

    // 2. Create new fragment with modified flags
    FEntityFlagFragment NewFlagFragment;
    NewFlagFragment.Flags = CurrentFlags | (1LL << static_cast<uint8>(FlagToSet));

    // 3. Call the generic "Set Fragment" function to replace the data
    // This is required because FEntityTemplateData is immutable-by-pointer
    Generic_SetFragmentInTemplate(TemplateData, FEntityFlagFragment::StaticStruct(), &NewFlagFragment);
}

void UMassAPIBPFnLib::ClearTemplateFlag(FEntityTemplateData& TemplateData, EEntityFlags FlagToClear)
{
    if (FlagToClear >= EEntityFlags::EEntityFlags_MAX)
    {
        return;
    }

    // 1. Get current flags
    const int64 CurrentFlags = GetTemplateFlags(TemplateData);

    // 2. Create new fragment with modified flags
    FEntityFlagFragment NewFlagFragment;
    NewFlagFragment.Flags = CurrentFlags & ~(1LL << static_cast<uint8>(FlagToClear));

    // 3. Call the generic "Set Fragment" function to replace the data
    Generic_SetFragmentInTemplate(TemplateData, FEntityFlagFragment::StaticStruct(), &NewFlagFragment);
}


//----------------------------------------------------------------------//
// (新) Flag Fragment Operations (已重构)
//----------------------------------------------------------------------//

int64 UMassAPIBPFnLib::GetEntityFlags(const UObject* WorldContextObject, const FEntityHandle& EntityHandle)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (!MassAPI)
    {
        return 0;
    }

    // FEntityHandle implicitly converts to FMassEntityHandle
    return MassAPI->GetEntityFlags(EntityHandle);
}

bool UMassAPIBPFnLib::HasEntityFlag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToTest)
{
    if (UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject))
    {
        return MassAPI->HasEntityFlag(EntityHandle, FlagToTest);
    }
    return false;
}

bool UMassAPIBPFnLib::SetEntityFlag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToSet)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (MassAPI)
    {
        return MassAPI->SetEntityFlag(EntityHandle, FlagToSet);
    }
    return false;
}

bool UMassAPIBPFnLib::ClearEntityFlag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToClear)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContextObject);
    if (MassAPI)
    {
        return MassAPI->ClearEntityFlag(EntityHandle, FlagToClear);
    }
    return false;
}

int64 UMassAPIBPFnLib::ConvertFlagsArrayToBitmask(const TArray<EEntityFlags>& Flags)
{
    int64 Bitmask = 0;
    for (const EEntityFlags Flag : Flags)
    {
        if (Flag < EEntityFlags::EEntityFlags_MAX)
        {
            Bitmask |= (1LL << static_cast<uint8>(Flag));
        }
    }
    return Bitmask;
}
