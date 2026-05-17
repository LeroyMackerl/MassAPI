/*
* MassAPI
* Created: 2025
* Author: Leroy Works, Ember, All Rights Reserved.
*
* Centralized version-conditional macros for UE 5.6 / 5.7 / 5.8+ | 集中式版本条件宏
* Every translation unit that includes MassAPIStructs.h or MassAPISubsystem.h gets these.
*/

#pragma once

#include "Runtime/Launch/Resources/Version.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Bit-set accessor macros (deprecated per-type bit sets → unified FMassElementBitSet) | 位集访问宏
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8
// UE 5.8+ unified bit set — Contains is on the composition descriptor directly.
// GetFragments() etc. exist but are deprecated and their Contains() returns void.
#define GET_TAGS GetElementsBitSet().Get<FMassTagBitSet>()
#define GET_FRAGMENTS GetElementsBitSet().Get<FMassFragmentBitSet>()
#define GET_CHUNK_FRAGMENTS GetElementsBitSet().Get<FMassChunkFragmentBitSet>()
#define GET_SHARED_FRAGMENTS GetElementsBitSet().Get<FMassSharedFragmentBitSet>()
#define GET_CONST_SHARED_FRAGMENTS GetElementsBitSet().Get<FMassConstSharedFragmentBitSet>()
#define CONTAINS_FRAGMENT(Comp, TypePtr)     Comp.Contains(TypePtr)
#define CONTAINS_TAG(Comp, TypePtr)          Comp.Contains(TypePtr)
#define CONTAINS_CHUNK(Comp, TypePtr)        Comp.Contains(TypePtr)
#define CONTAINS_SHARED(Comp, TypePtr)       Comp.Contains(TypePtr)
#define CONTAINS_CONST_SHARED(Comp, TypePtr) Comp.Contains(TypePtr)
#define CONTAINS_T_FRAGMENT(Comp, T)         Comp.Contains<T>()
#define CONTAINS_T_TAG(Comp, T)              Comp.Contains<T>()
#define CONTAINS_T_CHUNK(Comp, T)            Comp.Contains<T>()
#define CONTAINS_T_SHARED(Comp, T)           Comp.Contains<T>()
#define CONTAINS_T_CONST_SHARED(Comp, T)     Comp.Contains<T>()
#elif ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
// UE 5.7 accessor methods
#define GET_TAGS GetTags()
#define GET_FRAGMENTS GetFragments()
#define GET_CHUNK_FRAGMENTS GetChunkFragments()
#define GET_SHARED_FRAGMENTS GetSharedFragments()
#define GET_CONST_SHARED_FRAGMENTS GetConstSharedFragments()
#define CONTAINS_FRAGMENT(Comp, TypePtr)     Comp.GET_FRAGMENTS.Contains(*TypePtr)
#define CONTAINS_TAG(Comp, TypePtr)          Comp.GET_TAGS.Contains(*TypePtr)
#define CONTAINS_CHUNK(Comp, TypePtr)        Comp.GET_CHUNK_FRAGMENTS.Contains(*TypePtr)
#define CONTAINS_SHARED(Comp, TypePtr)       Comp.GET_SHARED_FRAGMENTS.Contains(*TypePtr)
#define CONTAINS_CONST_SHARED(Comp, TypePtr) Comp.GET_CONST_SHARED_FRAGMENTS.Contains(*TypePtr)
#define CONTAINS_T_FRAGMENT(Comp, T)         Comp.GET_FRAGMENTS.Contains<T>()
#define CONTAINS_T_TAG(Comp, T)              Comp.GET_TAGS.Contains<T>()
#define CONTAINS_T_CHUNK(Comp, T)            Comp.GET_CHUNK_FRAGMENTS.Contains<T>()
#define CONTAINS_T_SHARED(Comp, T)           Comp.GET_SHARED_FRAGMENTS.Contains<T>()
#define CONTAINS_T_CONST_SHARED(Comp, T)     Comp.GET_CONST_SHARED_FRAGMENTS.Contains<T>()
#else
// < UE 5.7 direct member access
#define GET_TAGS Tags
#define GET_FRAGMENTS Fragments
#define GET_CHUNK_FRAGMENTS ChunkFragments
#define GET_SHARED_FRAGMENTS SharedFragments
#define GET_CONST_SHARED_FRAGMENTS ConstSharedFragments
#define CONTAINS_FRAGMENT(Comp, TypePtr)     Comp.GET_FRAGMENTS.Contains(*TypePtr)
#define CONTAINS_TAG(Comp, TypePtr)          Comp.GET_TAGS.Contains(*TypePtr)
#define CONTAINS_CHUNK(Comp, TypePtr)        Comp.GET_CHUNK_FRAGMENTS.Contains(*TypePtr)
#define CONTAINS_SHARED(Comp, TypePtr)       Comp.GET_SHARED_FRAGMENTS.Contains(*TypePtr)
#define CONTAINS_CONST_SHARED(Comp, TypePtr) Comp.GET_CONST_SHARED_FRAGMENTS.Contains(*TypePtr)
#define CONTAINS_T_FRAGMENT(Comp, T)         Comp.GET_FRAGMENTS.Contains<T>()
#define CONTAINS_T_TAG(Comp, T)              Comp.GET_TAGS.Contains<T>()
#define CONTAINS_T_CHUNK(Comp, T)            Comp.GET_CHUNK_FRAGMENTS.Contains<T>()
#define CONTAINS_T_SHARED(Comp, T)           Comp.GET_SHARED_FRAGMENTS.Contains<T>()
#define CONTAINS_T_CONST_SHARED(Comp, T)     Comp.GET_CONST_SHARED_FRAGMENTS.Contains<T>()
#endif

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Version-conditional macros for Add / Remove / HasAll / HasAny | 版本条件宏
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8
// UE 5.8+ TNotNull API | 5.8+ TNotNull API
#define BIT_SET_ADD(BitSet, Ptr)              BitSet.Add(Ptr)
#define ENTITY_MANAGER_REMOVE_SHARED(Manager, Handle, Type)        (Manager)->RemoveSharedFragmentFromEntity(Handle, Type)
#define ENTITY_MANAGER_REMOVE_CONST_SHARED(Manager, Handle, Type)  (Manager)->RemoveConstSharedFragmentFromEntity(Handle, Type)
#define TEMPLATE_ADD_TAG(Template, Type)      Template.AddTag(Type)
#define TEMPLATE_ADD_FRAGMENT(Template, Type) Template.AddFragment(TNotNull<const UScriptStruct*>(Type))
#define TEMPLATE_REMOVE_TAG(Template, Type)   Template.RemoveTag(Type)
#define TEMPLATE_HAS_FRAGMENT(Data, Type)     Data->HasFragment(TNotNull<const UScriptStruct*>(Type))
#define TEMPLATE_HAS_SHARED(Data, Type)       Data->HasSharedFragment(TNotNull<const UScriptStruct*>(Type))
#define TEMPLATE_HAS_CONST_SHARED(Data, Type) Data->HasConstSharedFragment(TNotNull<const UScriptStruct*>(Type))
#define HAS_ALL_COMPOSITION(ThisComp, OtherComp)  (ThisComp).HasAll(OtherComp)
#define HAS_ANY_COMPOSITION(ThisComp, OtherComp)  (ThisComp).GetElementsBitSet().HasAny((OtherComp).GetElementsBitSet())
#elif ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
// UE 5.7 reference API | 5.7 引用 API
#define BIT_SET_ADD(BitSet, Ptr)              BitSet.Add(*(Ptr))
#define ENTITY_MANAGER_REMOVE_SHARED(Manager, Handle, Type)        (Manager)->RemoveSharedFragmentFromEntity(Handle, *(Type))
#define ENTITY_MANAGER_REMOVE_CONST_SHARED(Manager, Handle, Type)  (Manager)->RemoveConstSharedFragmentFromEntity(Handle, *(Type))
#define TEMPLATE_ADD_TAG(Template, Type)      Template.AddTag(*(Type))
#define TEMPLATE_ADD_FRAGMENT(Template, Type) Template.AddFragment(*(Type))
#define TEMPLATE_REMOVE_TAG(Template, Type)   Template.RemoveTag(*(Type))
#define TEMPLATE_HAS_FRAGMENT(Data, Type)     Data->HasFragment(*(Type))
#define TEMPLATE_HAS_SHARED(Data, Type)       Data->HasSharedFragment(*(Type))
#define TEMPLATE_HAS_CONST_SHARED(Data, Type) Data->HasConstSharedFragment(*(Type))
#define HAS_ALL_COMPOSITION(ThisComp, OtherComp) 	((ThisComp).GET_FRAGMENTS.HasAll((OtherComp).GET_FRAGMENTS) && 	 (ThisComp).GET_TAGS.HasAll((OtherComp).GET_TAGS) && 	 (ThisComp).GET_CHUNK_FRAGMENTS.HasAll((OtherComp).GET_CHUNK_FRAGMENTS) && 	 (ThisComp).GET_SHARED_FRAGMENTS.HasAll((OtherComp).GET_SHARED_FRAGMENTS) && 	 (ThisComp).GET_CONST_SHARED_FRAGMENTS.HasAll((OtherComp).GET_CONST_SHARED_FRAGMENTS))
#define HAS_ANY_COMPOSITION(ThisComp, OtherComp) 	((ThisComp).GET_FRAGMENTS.HasAny((OtherComp).GET_FRAGMENTS) || 	 (ThisComp).GET_TAGS.HasAny((OtherComp).GET_TAGS) || 	 (ThisComp).GET_CHUNK_FRAGMENTS.HasAny((OtherComp).GET_CHUNK_FRAGMENTS) || 	 (ThisComp).GET_SHARED_FRAGMENTS.HasAny((OtherComp).GET_SHARED_FRAGMENTS) || 	 (ThisComp).GET_CONST_SHARED_FRAGMENTS.HasAny((OtherComp).GET_CONST_SHARED_FRAGMENTS))
#else
// < UE 5.7 direct member access | 5.7 以下直接成员访问
#define BIT_SET_ADD(BitSet, Ptr)              BitSet.Add(*(Ptr))
#define ENTITY_MANAGER_REMOVE_SHARED(Manager, Handle, Type)        (Manager)->RemoveSharedFragmentFromEntity(Handle, *(Type))
#define ENTITY_MANAGER_REMOVE_CONST_SHARED(Manager, Handle, Type)  (Manager)->RemoveConstSharedFragmentFromEntity(Handle, *(Type))
#define TEMPLATE_ADD_TAG(Template, Type)      Template.AddTag(*(Type))
#define TEMPLATE_ADD_FRAGMENT(Template, Type) Template.AddFragment(*(Type))
#define TEMPLATE_REMOVE_TAG(Template, Type)   Template.RemoveTag(*(Type))
#define TEMPLATE_HAS_FRAGMENT(Data, Type)     Data->HasFragment(*(Type))
#define TEMPLATE_HAS_SHARED(Data, Type)       Data->HasSharedFragment(*(Type))
#define TEMPLATE_HAS_CONST_SHARED(Data, Type) Data->HasConstSharedFragment(*(Type))
#define HAS_ALL_COMPOSITION(ThisComp, OtherComp) 	((ThisComp).GET_FRAGMENTS.HasAll((OtherComp).GET_FRAGMENTS) && 	 (ThisComp).GET_TAGS.HasAll((OtherComp).GET_TAGS) && 	 (ThisComp).GET_CHUNK_FRAGMENTS.HasAll((OtherComp).GET_CHUNK_FRAGMENTS) && 	 (ThisComp).GET_SHARED_FRAGMENTS.HasAll((OtherComp).GET_SHARED_FRAGMENTS) && 	 (ThisComp).GET_CONST_SHARED_FRAGMENTS.HasAll((OtherComp).GET_CONST_SHARED_FRAGMENTS))
#define HAS_ANY_COMPOSITION(ThisComp, OtherComp) 	((ThisComp).GET_FRAGMENTS.HasAny((OtherComp).GET_FRAGMENTS) || 	 (ThisComp).GET_TAGS.HasAny((OtherComp).GET_TAGS) || 	 (ThisComp).GET_CHUNK_FRAGMENTS.HasAny((OtherComp).GET_CHUNK_FRAGMENTS) || 	 (ThisComp).GET_SHARED_FRAGMENTS.HasAny((OtherComp).GET_SHARED_FRAGMENTS) || 	 (ThisComp).GET_CONST_SHARED_FRAGMENTS.HasAny((OtherComp).GET_CONST_SHARED_FRAGMENTS))
#endif
