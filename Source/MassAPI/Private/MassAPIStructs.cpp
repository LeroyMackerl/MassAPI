#include "MassAPIStructs.h"

void FEntityTemplate::GetTemplateData(FMassEntityTemplateData& OutTemplateData, FMassEntityManager& EntityManager) const
{
    // Clear any existing data to ensure a fresh start
    OutTemplateData = FMassEntityTemplateData();

    // 1. Add Tags
    for (const FInstancedStruct& TagInstance : Tags)
    {
        if (TagInstance.IsValid())
        {
            OutTemplateData.AddTag(*TagInstance.GetScriptStruct());
        }
    }

    // 2. Add Fragments with initial values
    for (const FInstancedStruct& FragmentInstance : Fragments)
    {
        if (FragmentInstance.IsValid())
        {
            // AddFragment with FConstStructView will add the fragment to the composition 
            // and also store its initial value.
            OutTemplateData.AddFragment(FConstStructView(FragmentInstance));
        }
    }

    // 3. Add Mutable Shared Fragments
    for (const FInstancedStruct& SharedFragmentInstance : MutableSharedFragments)
    {
        if (SharedFragmentInstance.IsValid())
        {
            // The EntityManager is needed to create a handle to the shared data instance.
            // This ensures that all entities with the same shared fragment data point to the same memory.
            // We use the overload that takes a script struct and raw memory to avoid template ambiguity.
            const FSharedStruct& SharedStruct = EntityManager.GetOrCreateSharedFragment(*SharedFragmentInstance.GetScriptStruct(), SharedFragmentInstance.GetMemory());
            OutTemplateData.AddSharedFragment(SharedStruct);
        }
    }

    // 4. Add Const Shared Fragments
    for (const FInstancedStruct& ConstSharedFragmentInstance : ConstSharedFragments)
    {
        if (ConstSharedFragmentInstance.IsValid())
        {
            // Similar to mutable shared fragments, the EntityManager manages the instance.
            // We use the overload that takes a script struct and raw memory to avoid template ambiguity.
            const FConstSharedStruct& ConstSharedStruct = EntityManager.GetOrCreateConstSharedFragment(*ConstSharedFragmentInstance.GetScriptStruct(), ConstSharedFragmentInstance.GetMemory());
            OutTemplateData.AddConstSharedFragment(ConstSharedStruct);
        }
    }
}

