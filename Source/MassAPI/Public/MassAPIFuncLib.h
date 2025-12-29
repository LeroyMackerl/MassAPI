/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MassAPISubsystem.h"
#include "MassAPIEnums.h"
#include "MassAPIStructs.h"
#include "MassAPIFuncLib.generated.h"

// Forward declaration for the generic struct placeholder used in CustomThunks
struct FGenericStruct;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/**
 * A Blueprint function library providing utility functions for interacting with Mass entities.
 */
UCLASS()
class MASSAPI_API UMassAPIFuncLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	//================ Entity Operations																		========

	/**
	 * Checks if the provided entity handle refers to a valid and active entity.
	 * @param WorldContextObject The context object to retrieve the world.
	 * @param EntityHandle The handle of the entity to check.
	 * @return True if the entity is valid and active, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", BlueprintPure, meta = (WorldContext = "WorldContextObject", DisplayName = "IsValid (EntityHandle)", Tooltip = "Checks if the provided entity handle refers to a valid and active entity.", Keywords = "valid isvalid mass entity check active exists"))
	static bool IsValid(const UObject* WorldContextObject, const FEntityHandle& EntityHandle);

	//———————— Destroy.Entity																						————

	/**
	 * Destroys a specific entity.
	 * @param WorldContextObject The context object to retrieve the world.
	 * @param EntityHandle The handle of the entity to destroy.
	 * @param bDeferred If true, the destruction is queued via the command buffer; otherwise, it happens immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Destroy Entity", Tooltip = "Destroys a specific entity.", Keywords = "destroy delete remove kill mass entity"))
	static void DestroyEntity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, const bool bDeferred);

	/**
	 * Destroys a batch of entities.
	 * @param WorldContextObject The context object to retrieve the world.
	 * @param EntityHandles An array of entity handles to destroy.
	 * @param bDeferred If true, the destruction is queued via the command buffer; otherwise, it happens immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Destroy Entities", Tooltip = "Destroys a batch of entities.", Keywords = "destroy delete remove kill batch mass entity array"))
	static void DestroyEntities(const UObject* WorldContextObject, const TArray<FEntityHandle>& EntityHandles, const bool bDeferred);

	//================ Entity Building																			========

	/**
	 * Builds a single entity based on the provided template data.
	 * @param WorldContextObject The context object to retrieve the world.
	 * @param TemplateData The template data defining the entity's composition and initial values.
	 * @param bDeferred If true, creation is queued; otherwise, it happens immediately.
	 * @return The handle to the newly created (or reserved) entity.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Build Entity From Template Data", Tooltip = "Builds a single entity based on the provided template data.", Keywords = "spawn create make construct build mass entity template"))
	static FEntityHandle BuildEntityFromTemplateData(const UObject* WorldContextObject, UPARAM(ref) const FEntityTemplateData& TemplateData, const bool bDeferred);

	/**
	 * Builds multiple entities based on the provided template data.
	 * @param WorldContextObject The context object to retrieve the world.
	 * @param Quantity The number of entities to spawn.
	 * @param TemplateData The template data defining the entities' composition and initial values.
	 * @param bDeferred If true, creation is queued; otherwise, it happens immediately.
	 * @return An array of handles to the newly created (or reserved) entities.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Build Entities From Template Data", Tooltip = "Builds multiple entities based on the provided template data.", Keywords = "spawn create make construct build batch mass entity template array"))
	static TArray<FEntityHandle> BuildEntitiesFromTemplateData(const UObject* WorldContextObject, int32 Quantity, UPARAM(ref) const FEntityTemplateData& TemplateData, const bool bDeferred);

	//================ Entity Querying & BP Processors															========

	/**
	 * Checks if a specific entity matches the provided query criteria.
	 * @param WorldContextObject The context object to retrieve the world.
	 * @param EntityHandle The entity to check.
	 * @param Query The query rules (All, Any, None tags/fragments/flags).
	 * @return True if the entity matches the query, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Query", BlueprintPure, meta = (WorldContext = "WorldContextObject", DisplayName = "Match Entity Query", Tooltip = "Checks if a specific entity matches the provided query criteria.", Keywords = "match query filter check mass entity"))
	static bool MatchEntityQuery(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(ref) const FEntityQuery& Query);

	/**
	 * Counts how many entities in the world match the provided query.
	 * @param WorldContextObject The context object to retrieve the world.
	 * @param Query The query rules.
	 * @return The number of matching entities.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Query", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Num Matching Entities", Tooltip = "Counts how many entities in the world match the provided query.", Keywords = "get count num number query filter mass entity entities"))
	static int32 GetNumMatchingEntities(const UObject* WorldContextObject, UPARAM(ref) const FEntityQuery& Query);

	/**
	 * Retrieves all entities that match the provided query.
	 * Warning: This can return a large array and impact performance if the result set is huge.
	 * @param WorldContextObject The context object to retrieve the world.
	 * @param Query The query rules.
	 * @return An array of handles for all matching entities.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Query", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Matching Entities", Tooltip = "Retrieves all entities that match the provided query. Use it for prototyping only, because for each loop in BP is slow.", Keywords = "get find query filter mass entity entities array list"))
	static TArray<FEntityHandle> GetMatchingEntities(const UObject* WorldContextObject, UPARAM(ref) const FEntityQuery& Query);

	/**
	 * Returns the MassAPI subsystem to perform iteration.
	 * Used internally by the "For Each Matching Entity" macro/node logic.
	 * @param WorldContextObject The context object to retrieve the world.
	 * @return A pointer to the MassAPI subsystem.
	 */
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = "MassAPI|Query", meta = (WorldContext = "WorldContextObject", Tooltip = "Returns the MassAPI subsystem to perform iteration.", Keywords = "loop iterate foreach query mass entity"))
	static class UMassAPISubsystem* ForEachMatchingEntities(const UObject* WorldContextObject);

	//================ TemplateData Operations																	========

	/**
	 * Converts a Blueprint-defined Entity Template struct into runtime Template Data.
	 * @param WorldContextObject The context object to retrieve the world.
	 * @param Template The source Blueprint template struct.
	 * @return The compiled entity template data ready for spawning.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Template", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Template Data", Tooltip = "Converts a Blueprint-defined Entity Template struct into runtime Template Data.", Keywords = "get make retrieve template data mass"))
	static FEntityTemplateData GetTemplateData(const UObject* WorldContextObject, UPARAM(ref) const FEntityTemplate& Template);

	/**
	 * Auto-cast converter to transform a Template struct into Template Data.
	 * @param WorldContextObject The context object.
	 * @param Template The source template.
	 * @return The converted template data.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Template", BlueprintPure, meta = (WorldContext = "WorldContextObject", DisplayName = "Template To Template Data", CompactNodeTitle = "->", BlueprintAutocast, Tooltip = "Auto-cast converter to transform a Template struct into Template Data.", Keywords = "convert cast template data mass auto"))
	static FEntityTemplateData Conv_TemplateToTemplateData(const UObject* WorldContextObject, UPARAM(ref) const FEntityTemplate& Template);

	/**
	 * Checks if the template data is empty or invalid.
	 * @param TemplateData The template data to check.
	 * @return True if the data is empty or invalid, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Template", BlueprintPure, meta = (DisplayName = "IsEmpty (TemplateData)", Tooltip = "Checks if the template data is empty or invalid.", Keywords = "check is empty valid template data mass"))
	static bool IsEmpty_TemplateData(UPARAM(ref) const FEntityTemplateData& TemplateData);

	//================ Fragment Operations																		========

	//———————— Set.Fragment.Entity (Unified for Fragment/SharedFragment/ConstSharedFragment)								————

	/**
	 * Unified function to set a Fragment, SharedFragment, or ConstSharedFragment on an entity.
	 * The node automatically adapts to the type of struct connected to 'InFragment'.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to modify.
	 * @param FragmentType The script struct of the fragment.
	 * @param InFragment The value to set.
	 * @param bSuccess Output indicating if the operation succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", CustomThunk, BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment", Tooltip = "Unified function to set a Fragment on an entity.", Keywords = "set add update fragment unified mass entity value"))
	static void SetFragment_Entity_Unified(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool bDeferred, bool& bSuccess);

	/**
	 * Generic implementation for setting a fragment on an entity.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to modify.
	 * @param FragmentType The type of the fragment.
	 * @param InFragmentPtr Pointer to the raw memory of the fragment value.
	 * @param bSuccess Output indicating success.
	 */
	static void Generic_SetFragment_Entity_Unified(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const void* InFragmentPtr, bool bDeferred, bool& bSuccess);
	DECLARE_FUNCTION(execSetFragment_Entity_Unified);

	//———————— Set.Fragment.Template (Unified for Fragment/SharedFragment/ConstSharedFragment)						————

	/**
	 * Unified function to set a Fragment, SharedFragment, or ConstSharedFragment in Template Data.
	 * @param WorldContextObject The context object.
	 * @param TemplateData The template data to modify.
	 * @param FragmentType The script struct of the fragment.
	 * @param InFragment The value to set.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", CustomThunk, BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment", Tooltip = "Unified function to set a Fragment, SharedFragment, or ConstSharedFragment in Template Data. Will add a new one or override the the existing one.", Keywords = "set add update fragment shared const unified template mass value"))
	static void SetFragment_Template_Unified(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const FGenericStruct& InFragment);

	/**
	 * Generic implementation for setting a fragment in template data.
	 * @param WorldContextObject The context object.
	 * @param TemplateData The template data to modify.
	 * @param FragmentType The type of the fragment.
	 * @param InFragmentPtr Pointer to the raw memory of the fragment value.
	 */
	static void Generic_SetFragment_Template_Unified(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const void* InFragmentPtr);
	DECLARE_FUNCTION(execSetFragment_Template_Unified);

	//———————— Get.Fragment.Entity (Unified for Fragment/SharedFragment/ConstSharedFragment)								————

	/**
	 * Unified function to retrieve a Fragment, SharedFragment, or ConstSharedFragment from an entity.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to read from.
	 * @param FragmentType The script struct of the fragment to retrieve.
	 * @param OutFragment Output struct to hold the retrieved data.
	 * @param bSuccess Output indicating if the fragment was found and retrieved.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", BlueprintPure, CustomThunk, BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment", Tooltip = "Unified function to retrieve a Fragment, SharedFragment, or ConstSharedFragment from an entity.", Keywords = "get read retrieve getfragment gf mass entity fragment shared const unified value"))
	static void GetFragment_Entity_Unified(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);

	/**
	 * Generic implementation for getting a fragment from an entity.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to read from.
	 * @param FragmentType The type of the fragment.
	 * @param OutFragmentPtr Pointer to the memory where the result should be copied.
	 * @param bSuccess Output indicating success.
	 */
	static void Generic_GetFragment_Entity_Unified(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, void* OutFragmentPtr, bool& bSuccess);
	DECLARE_FUNCTION(execGetFragment_Entity_Unified);

	//———————— Get.Fragment.Template																				————

	/**
	 * Unified function to retrieve a Fragment, SharedFragment, or ConstSharedFragment from Template Data.
	 * @param TemplateData The template data to read from.
	 * @param FragmentType The script struct of the fragment to retrieve.
	 * @param OutFragment Output struct to hold the retrieved data.
	 * @param bSuccess Output indicating if the fragment was found.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", BlueprintPure, CustomThunk, BlueprintInternalUseOnly, meta = (CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment", Tooltip = "Unified function to retrieve a Fragment, SharedFragment, or ConstSharedFragment from Template Data.", Keywords = "get read retrieve fragment shared const unified template mass value"))
	static void GetFragment_Template_Unified(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);

	/**
	 * Generic implementation for getting a fragment from template data.
	 * @param TemplateData The template data to read from.
	 * @param FragmentType The type of the fragment.
	 * @param OutFragmentPtr Pointer to the memory where the result should be copied.
	 * @param bSuccess Output indicating success.
	 */
	static void Generic_GetFragment_Template_Unified(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, void* OutFragmentPtr, bool& bSuccess);
	DECLARE_FUNCTION(execGetFragment_Template_Unified);

	//———————— Remove.Fragment.Entity (Unified for Fragment/SharedFragment/ConstSharedFragment)							————

	/**
	 * Removes a Fragment, SharedFragment, or ConstSharedFragment from an entity.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to modify.
	 * @param FragmentType The type of fragment to remove.
	 * @return True if the fragment was successfully removed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", Tooltip = "Removes a Fragment from an entity.", Keywords = "remove delete clear fragment mass entity"))
	static bool RemoveFragment_Entity_Unified(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, bool bDeferred = false);

	//———————— Remove.Fragment.Template (Unified for Fragment/SharedFragment/ConstSharedFragment)					————

	/**
	 * Removes a Fragment, SharedFragment, or ConstSharedFragment from Template Data.
	 * @param WorldContextObject The context object.
	 * @param TemplateData The template data to modify.
	 * @param FragmentType The type of fragment to remove.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", Tooltip = "Removes a Fragment, SharedFragment, or ConstSharedFragment from Template Data.", Keywords = "remove delete clear fragment shared const unified template mass"))
	static void RemoveFragment_Template_Unified(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType);

	//———————— Has.Fragment.Entity																							————

	/**
	 * Checks if an entity has a specific Fragment, SharedFragment, or ConstSharedFragment.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to check.
	 * @param FragmentType The type of fragment to check for.
	 * @return True if the entity has the fragment, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", BlueprintInternalUseOnly, BlueprintPure, meta = (WorldContext = "WorldContextObject", Tooltip = "Checks if an entity has a specific Fragment, SharedFragment, or ConstSharedFragment.", Keywords = "has check exist fragment shared const unified mass entity"))
	static bool HasFragment_Entity_Unified(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType);

	//———————— Has.Fragment.Template																				————

	/**
	 * Checks if Template Data contains a specific Fragment, SharedFragment, or ConstSharedFragment.
	 * @param TemplateData The template data to check.
	 * @param FragmentType The type of fragment to check for.
	 * @return True if the template contains the fragment, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", BlueprintInternalUseOnly, BlueprintPure, meta = (WorldContext = "WorldContextObject", Tooltip = "Checks if Template Data contains a specific Fragment, SharedFragment, or ConstSharedFragment.", Keywords = "has check exist fragment shared const unified template mass"))
	static bool HasFragment_Template_Unified(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType);

	//================ Tag Operations																			========

	//———————— Add.Tag.Entity																								————

	/**
	 * Adds a Tag to an entity.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to modify.
	 * @param TagType The type of tag to add.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", Tooltip = "Adds a Tag to an entity.", Keywords = "add set tag mass entity"))
	static void AddTag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType, bool bDeferred = false);

	//———————— Add.Tag.Template																						————

	/**
	 * Adds a Tag to Template Data.
	 * @param TemplateData The template data to modify.
	 * @param TagType The type of tag to add.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", BlueprintInternalUseOnly, meta = (Tooltip = "Adds a Tag to Template Data.", Keywords = "add set tag template mass"))
	static void AddTag_Template(UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* TagType);

	//———————— Remove.Tag.Entity																							————

	/**
	 * Removes a Tag from an entity.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to modify.
	 * @param TagType The type of tag to remove.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", Tooltip = "Removes a Tag from an entity.", Keywords = "remove delete clear tag mass entity"))
	static void RemoveTag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType, bool bDeferred = false);

	//———————— Remove.Tag.Template																					————

	/**
	 * Removes a Tag from Template Data.
	 * @param TemplateData The template data to modify.
	 * @param TagType The type of tag to remove.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", BlueprintInternalUseOnly, meta = (Tooltip = "Removes a Tag from Template Data.", Keywords = "remove delete clear tag template mass"))
	static void RemoveTag_Template(UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* TagType);

	//———————— Has.Tag.Entity																								————

	/**
	 * Checks if an entity has a specific Tag.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to check.
	 * @param TagType The type of tag to check for.
	 * @return True if the entity has the tag, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", BlueprintInternalUseOnly, BlueprintPure, meta = (WorldContext = "WorldContextObject", Tooltip = "Checks if an entity has a specific Tag.", Keywords = "has check exist tag mass entity"))
	static bool HasTag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType);

	//———————— Has.Tag.Template																						————

	/**
	 * Checks if Template Data contains a specific Tag.
	 * @param TemplateData The template data to check.
	 * @param TagType The type of tag to check for.
	 * @return True if the template contains the tag, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Composition", BlueprintInternalUseOnly, BlueprintPure, meta = (WorldContext = "WorldContextObject", Tooltip = "Checks if Template Data contains a specific Tag.", Keywords = "has check exist tag template mass"))
	static bool HasTag_Template(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* TagType);

	//================ Flag Operations																			========

	//———————— Has.Flag.Entity																								————

	/**
	 * Checks if a specific Entity Flag is set on an entity.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to check.
	 * @param FlagToTest The flag enum value to check for.
	 * @return True if the flag is set, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Flag", BlueprintInternalUseOnly, BlueprintPure, meta = (WorldContext = "WorldContextObject", Tooltip = "Requires the entity to have FEntityFlagFragment added via its Template.", Keywords = "has check exist flag bitmask mass entity"))
	static bool HasFlag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToTest);

	//———————— Has.Flag.Template																					————

	/**
	 * Checks if a specific Entity Flag is set in Template Data.
	 * @param TemplateData The template data to check.
	 * @param FlagToTest The flag enum value to check for.
	 * @return True if the flag is set, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Flag", BlueprintInternalUseOnly, BlueprintPure, meta = (WorldContext = "WorldContextObject", Tooltip = "Requires the entity to have FEntityFlagFragment added via its Template.", Keywords = "has check exist flag bitmask template mass"))
	static bool HasFlag_Template(UPARAM(ref) const FEntityTemplateData& TemplateData, EEntityFlags FlagToTest);

	//———————— Get.Flags.Entity																							————

	/**
	 * Retrieves the raw integer bitmask of flags from an entity.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to read from.
	 * @return The bitmask representing all set flags.
	 */
	static int64 GetFlag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle);

	//———————— Get.Flags.Template																					————

	/**
	 * Retrieves the raw integer bitmask of flags from Template Data.
	 * @param TemplateData The template data to read from.
	 * @return The bitmask representing all set flags in the template.
	 */
	static int64 GetFlag_Template(UPARAM(ref) const FEntityTemplateData& TemplateData);

	//———————— Set.Flag.Entity																								————

	/**
	 * Sets a specific Entity Flag (bitmask) on an entity.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to modify.
	 * @param FlagToSet The flag enum value to set.
	 * @return True if the flag was set, false if the entity is invalid or lacks the required flag fragment.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Flag", BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", Tooltip = "Requires the entity to have FEntityFlagFragment added via its Template.", Keywords = "set add update flag bitmask mass entity"))
	static bool SetFlag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToSet);

	//———————— Set.Flag.Template																					————

	/**
	 * Sets a specific Entity Flag (bitmask) in Template Data.
	 * @param WorldContextObject The context object.
	 * @param TemplateData The template data to modify.
	 * @param FlagToSet The flag enum value to set.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Flag", BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", Tooltip = "Requires the entity to have FEntityFlagFragment added via its Template.", Keywords = "set add update flag bitmask template mass"))
	static void SetFlag_Template(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, EEntityFlags FlagToSet);

	//———————— Clear.Flag.Entity																							————

	/**
	 * Clears a specific Entity Flag on an entity.
	 * @param WorldContextObject The context object.
	 * @param EntityHandle The entity to modify.
	 * @param FlagToClear The flag enum value to clear.
	 * @return True if successful, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Flag", BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", Tooltip = "Requires the entity to have FEntityFlagFragment added via its Template.", Keywords = "clear remove delete flag bitmask mass entity"))
	static bool ClearFlag_Entity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToClear);

	//———————— Clear.Flag.Template																					————

	/**
	 * Clears a specific Entity Flag in Template Data.
	 * @param WorldContextObject The context object.
	 * @param TemplateData The template data to modify.
	 * @param FlagToClear The flag enum value to clear.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Flag", BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", Tooltip = "Requires the entity to have FEntityFlagFragment added via its Template.", Keywords = "clear remove delete flag bitmask template mass"))
	static void ClearFlag_Template(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, EEntityFlags FlagToClear);



	//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
	// Deprecated Functions
	//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

public:

	//———————————————————Tag Ops

	/**
	 * Deprecated. Use HasTag_Entity instead.
	 * Checks if an entity has a specific Tag.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "HasMassTag(Deprecated)", BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static bool HasTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType);

	/**
	 * Deprecated. Use AddTag_Entity instead.
	 * Adds a Tag to an entity.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "AddMassTag(Deprecated)", meta = (WorldContext = "WorldContextObject"))
	static void AddTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType);

	/**
	 * Deprecated. Use RemoveTag_Entity instead.
	 * Removes a Tag from an entity.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "RemoveMassTag(Deprecated)", meta = (WorldContext = "WorldContextObject"))
	static void RemoveTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* TagType);

	/**
	 * Deprecated. Use HasTag_Template instead.
	 * Checks if Template Data contains a specific Tag.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "HasMassTag_Template(Deprecated)", BlueprintPure, meta = ())
	static bool HasTag_TemplateData(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* TagType);

	/**
	 * Deprecated. Use AddTag_Template instead.
	 * Adds a Tag to Template Data.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "AddMassTag_Template(Deprecated)", meta = ())
	static void AddTag_TemplateData(UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* TagType);

	/**
	 * Deprecated. Use RemoveTag_Template instead.
	 * Removes a Tag from Template Data.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "RemoveMassTag_Template(Deprecated)", meta = ())
	static void RemoveTag_TemplateData(UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* TagType);

	//———————————————————Fragment Ops

	/**
	 * Deprecated. Use HasFragment_Entity_Unified instead.
	 * Checks if an entity has a specific Fragment.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "HasMassFragment(Deprecated)", BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static bool HasFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType);

	/**
	 * Deprecated. Use RemoveFragment_Entity_Unified instead.
	 * Removes a Fragment from an entity.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "RemoveMassFragment(Deprecated)", meta = (WorldContext = "WorldContextObject"))
	static bool RemoveFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType);

	/**
	 * Deprecated. Use GetFragment_Entity_Unified instead.
	 * Gets a Fragment value from an entity.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", BlueprintPure, CustomThunk, BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment"))
	static void GetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);
	DECLARE_FUNCTION(execGetFragment);

	/**
	 * Deprecated. Use SetFragment_Entity_Unified instead.
	 * Sets a Fragment value on an entity.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", CustomThunk, BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment"))
	static void SetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess);
	DECLARE_FUNCTION(execSetFragment);

	/**
	 * Deprecated. Use HasFragment_Template_Unified instead.
	 * Checks if Template Data contains a specific Fragment.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "HasMassFragment_Template(Deprecated)", BlueprintPure, meta = ())
	static bool HasFragment_TemplateData(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType);

	/**
	 * Deprecated. Use RemoveFragment_Template_Unified instead.
	 * Removes a Fragment from Template Data.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "RemoveMassFragment_Template(Deprecated)", meta = ())
	static void RemoveFragmentInTemplate(UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType);

	/**
	 * Deprecated. Use GetFragment_Template_Unified instead.
	 * Gets a Fragment value from Template Data.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", BlueprintPure, CustomThunk, BlueprintInternalUseOnly, meta = (CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment"))
	static void GetFragmentFromTemplate(UPARAM(ref) const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);
	DECLARE_FUNCTION(execGetFragmentFromTemplate);

	/**
	 * Deprecated. Use SetFragment_Template_Unified instead.
	 * Sets a Fragment value in Template Data.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", CustomThunk, BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment"))
	static void SetFragmentInTemplate(UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const FGenericStruct& InFragment);
	DECLARE_FUNCTION(execSetFragmentInTemplate);

	//———————————————————SharedFragment Ops

	// Missing HasSharedFragment

	/**
	 * Deprecated. Use RemoveFragment_Entity_Unified instead.
	 * Removes a Shared Fragment from an entity.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "RemoveSharedFragment(Deprecated)", meta = (WorldContext = "WorldContextObject"))
	static bool RemoveSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType);

	/**
	 * Deprecated. Use GetFragment_Entity_Unified instead.
	 * Gets a Shared Fragment value from an entity.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", BlueprintPure, CustomThunk, BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment"))
	static void GetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);
	DECLARE_FUNCTION(execGetSharedFragment);

	/**
	 * Deprecated. Use SetFragment_Entity_Unified instead.
	 * Sets a Shared Fragment value on an entity.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", CustomThunk, BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment"))
	static void SetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess);
	DECLARE_FUNCTION(execSetSharedFragment);

	// Missing HasSharedFragmentInTemplate

	/**
	 * Deprecated. Use RemoveFragment_Template_Unified instead.
	 * Removes a Shared Fragment from Template Data.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "RemoveSharedFragment_Template(Deprecated)", meta = (WorldContext = "WorldContextObject"))
	static void RemoveSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType);

	// Missing GetSharedFragmentInTemplate

	/**
	 * Deprecated. Use SetFragment_Template_Unified instead.
	 * Sets a Shared Fragment value in Template Data.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", CustomThunk, BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment"))
	static void SetSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const FGenericStruct& InFragment);
	DECLARE_FUNCTION(execSetSharedFragmentInTemplate);


	//———————————————————ConstSharedFragment Ops

	// Missing HasConstSharedFragment

	/**
	 * Deprecated. Use RemoveFragment_Entity_Unified instead.
	 * Removes a Const Shared Fragment from an entity.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "RemoveConstSharedFragment(Deprecated)", meta = (WorldContext = "WorldContextObject"))
	static bool RemoveConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType);

	/**
	 * Deprecated. Use GetFragment_Entity_Unified instead.
	 * Gets a Const Shared Fragment value from an entity.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", BlueprintPure, CustomThunk, BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment"))
	static void GetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);
	DECLARE_FUNCTION(execGetConstSharedFragment);

	/**
	 * Deprecated. Use SetFragment_Entity_Unified instead.
	 * Sets a Const Shared Fragment value on an entity.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", CustomThunk, BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment"))
	static void SetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess);
	DECLARE_FUNCTION(execSetConstSharedFragment);

	// Missing HasSharedFragmentInTemplate

	/**
	 * Deprecated. Use RemoveFragment_Template_Unified instead.
	 * Removes a Const Shared Fragment from Template Data.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "RemoveConstSharedFragment_Template(Deprecated)", meta = (WorldContext = "WorldContextObject"))
	static void RemoveConstSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType);

	// Missing GetSharedFragmentInTemplate

	/**
	 * Deprecated. Use SetFragment_Template_Unified instead.
	 * Sets a Const Shared Fragment value in Template Data.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", CustomThunk, BlueprintInternalUseOnly, meta = (WorldContext = "WorldContextObject", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment"))
	static void SetConstSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const FGenericStruct& InFragment);
	DECLARE_FUNCTION(execSetConstSharedFragmentInTemplate);

	//———————————————————Flag Ops

	// --- Entity Flag Renames ---

	/**
	 * Deprecated. Use HasFlag_Entity instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "HasEntityFlag(Deprecated)", BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static bool HasEntityFlag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToTest);

	/**
	 * Deprecated. Use ClearFlag_Entity instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "ClearEntityFlag(Deprecated)", meta = (WorldContext = "WorldContextObject"))
	static bool ClearEntityFlag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToClear);

	/**
	 * Deprecated. Use SetFlag_Entity instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "SetEntityFlag(Deprecated)", meta = (WorldContext = "WorldContextObject"))
	static bool SetEntityFlag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToSet);

	// --- Template Data Renames ---

	/**
	 * Deprecated. Use HasFlag_Template instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "HasEntityFlag_Template(Deprecated)", BlueprintPure, meta = ())
	static bool HasTemplateFlag(UPARAM(ref) const FEntityTemplateData& TemplateData, EEntityFlags FlagToTest);

	/**
	 * Deprecated. Use ClearFlag_Template instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "ClearEntityFlag_Template(Deprecated)", meta = ())
	static void ClearTemplateFlag(UPARAM(ref) FEntityTemplateData& TemplateData, EEntityFlags FlagToClear);

	/**
	 * Deprecated. Use SetFlag_Template instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|ZDeprecated", DisplayName = "SetEntityFlag_Template(Deprecated)", meta = ())
	static void SetTemplateFlag(UPARAM(ref) FEntityTemplateData& TemplateData, EEntityFlags FlagToSet);

};