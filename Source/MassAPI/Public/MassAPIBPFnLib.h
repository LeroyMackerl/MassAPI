#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MassAPISubsystem.h"
#include "MassAPIBPFnLib.generated.h"

// Forward declaration for the generic struct placeholder used in CustomThunks
struct FGenericStruct;

/**
 * A Blueprint function library providing utility functions for interacting with Mass entities.
 */
UCLASS()
class MASSAPI_API UMassAPIBPFnLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	//----------------------------------------------------------------------//
	// Entity Operations
	//----------------------------------------------------------------------//

	/**
	 * Checks if the entity handle is valid and the corresponding entity is alive and fully created.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "IsValid (EntityHandle)", Keywords = "valid isvalid mass entity"))
	static bool IsValid(const UObject* WorldContextObject, const FEntityHandle& EntityHandle);

	/**
	 * Checks if an entity has a specific fragment.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Has Mass Fragment", Keywords = "has hasfragment mass entity fragment"))
	static bool HasFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassFragment")) UScriptStruct* FragmentType);

	/**
	 * Checks if an entity has a specific tag.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Has Mass Tag", Keywords = "has hastag mass entity tag"))
	static bool HasTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassTag")) UScriptStruct* TagType);

	/**
	 * Gets a copy of a fragment's data from an entity.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, CustomThunk, BlueprintInternalUseOnly, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Mass Fragment", CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment", Keywords = "get getfragment gf mass entity fragment"))
	static void GetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);

	/**
	 * Sets the data for an existing fragment on an entity, or adds it if it doesn't exist.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Mass Fragment", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment", Keywords = "set setfragment sf mass entity fragment"))
	static void SetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassFragment")) UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess);

	/**
	 * Adds a tag to an entity. Does nothing if the tag already exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Add Mass Tag", Keywords = "add tag mass entity"))
	static void AddTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassTag")) UScriptStruct* TagType);

	/**
	 * Synchronously destroys an entity.
	 * @param WorldContextObject The world context.
	 * @param EntityHandle The handle to the entity to destroy.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Destroy Entity", Keywords = "destroy delete remove mass entity"))
	static void DestroyEntity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle);

	//----------------------------------------------------------------------//
	// Batch Entity Operations
	//----------------------------------------------------------------------//

	/**
	 * Synchronously destroys multiple entities.
	 * @param WorldContextObject The world context.
	 * @param EntityHandles An array of entity handles to destroy.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Destroy Entities", Keywords = "destroy delete remove batch mass entity"))
	static void DestroyEntities(const UObject* WorldContextObject, const TArray<FEntityHandle>& EntityHandles);

	// Batch Add/Remove Tag removed as requested

	//----------------------------------------------------------------------//
	// Shared Fragment Operations
	//----------------------------------------------------------------------//

	/**
	 * Gets a copy of a shared fragment's data from an entity.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, CustomThunk, BlueprintInternalUseOnly, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Shared Mass Fragment", CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment", Keywords = "get shared mass entity fragment"))
	static void GetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);

	/**
	 * Sets the shared fragment for an entity. This might change the entity's archetype.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Shared Mass Fragment", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment", Keywords = "set shared mass entity fragment"))
	static void SetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassSharedFragment")) UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess);

	//----------------------------------------------------------------------//
	// Const Shared Fragment Operations
	//----------------------------------------------------------------------//

	/**
	 * Gets a copy of a const shared fragment's data from an entity.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, CustomThunk, BlueprintInternalUseOnly, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Const Shared Mass Fragment", CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment", Keywords = "get const shared mass entity fragment"))
	static void GetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);

	/**
	 * Sets the const shared fragment for an entity. This might change the entity's archetype.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Const Shared Mass Fragment", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment", Keywords = "set const shared mass entity fragment"))
	static void SetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassConstSharedFragment")) UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess);


	//----------------------------------------------------------------------//
	// Entity Building
	//----------------------------------------------------------------------//

	/**
	 * Synchronously builds a single entity from the given template data.
	 * @param WorldContextObject The world context.
	 * @param TemplateData The template data to use for building the entity.
	 * @return The handle to the newly created entity. Will be invalid if building fails.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Build Entity From Template Data"))
	static FEntityHandle BuildEntityFromTemplateData(const UObject* WorldContextObject, const FEntityTemplateData& TemplateData);

	/**
	 * Synchronously builds multiple entities from the given template data.
	 * @param WorldContextObject The world context.
	 * @param Quantity The number of entities to build.
	 * @param TemplateData The template data to use for building the entities.
	 * @return An array of handles to the newly created entities.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mass|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Build Entities From Template Data"))
	static TArray<FEntityHandle> BuildEntitiesFromTemplateData(const UObject* WorldContextObject, int32 Quantity, const FEntityTemplateData& TemplateData);


	//----------------------------------------------------------------------//
	// TemplateData Operations
	//----------------------------------------------------------------------//

	/**
	 * Converts an FEntityTemplate into a modifiable FEntityTemplateData handle.
	 * This creates a new template data instance that can be edited with other nodes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mass|Template", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Template Data"))
	static FEntityTemplateData GetTemplateData(const UObject* WorldContextObject, const FEntityTemplate& Template);

	/**
	 * Checks if the template data is empty (contains no fragments or tags).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mass|Template", meta = (DisplayName = "IsEmpty (TemplateData)"))
	static bool IsEmpty_TemplateData(const FEntityTemplateData& TemplateData);

	/**
	 * Checks if the template data contains a specific fragment type.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mass|Template", meta = (DisplayName = "Has Fragment (TemplateData)"))
	static bool HasFragment_TemplateData(const FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassFragment")) UScriptStruct* FragmentType);

	/**
	 * Adds a tag to the template data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mass|Template", meta = (DisplayName = "Add Tag (TemplateData)"))
	static void AddTag_TemplateData(UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassTag")) UScriptStruct* TagType);

	/**
	 * Removes a tag from the template data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mass|Template", meta = (DisplayName = "Remove Tag (TemplateData)"))
	static void RemoveTag_TemplateData(UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassTag")) UScriptStruct* TagType);

	/**
	 * Checks if the template data contains a specific tag type.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mass|Template", meta = (DisplayName = "Has Tag (TemplateData)"))
	static bool HasTag_TemplateData(const FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassTag")) UScriptStruct* TagType);

	/**
	 * Gets a copy of a fragment's initial value from the template data.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, CustomThunk, BlueprintInternalUseOnly, Category = "Mass|Template", meta = (DisplayName = "Get Fragment from Template", CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment"))
	static void GetFragmentFromTemplate(const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);

	/**
	 * Adds or sets a fragment's initial value in the template data.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "Mass|Template", meta = (DisplayName = "Set Fragment in Template", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment"))
	static void SetFragmentInTemplate(UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassFragment")) UScriptStruct* FragmentType, const FGenericStruct& InFragment);

	/**
	 * Adds or sets a shared fragment's value in the template data.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "Mass|Template", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Shared Fragment in Template", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment"))
	static void SetSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassSharedFragment")) UScriptStruct* FragmentType, const FGenericStruct& InFragment);

	/**
	 * Adds or sets a const shared fragment's value in the template data.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "Mass|Template", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Const Shared Fragment in Template", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment"))
	static void SetConstSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassConstSharedFragment")) UScriptStruct* FragmentType, const FGenericStruct& InFragment);


	//----------------------------------------------------------------------//
	// Entity Querying & BP Processors
	//----------------------------------------------------------------------//

	/**
	 * Checks if an entity's composition matches all the requirements of an FEntityQuery.
	 * @param WorldContextObject The world context.
	 * @param EntityHandle The entity to check.
	 * @param Query The query to match against.
	 * @return True if the entity matches the query (passes All, Any, and None requirements), false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mass|Query", meta = (WorldContext = "WorldContextObject", DisplayName = "Match Entity Query", Keywords = "match query filter mass entity"))
	static bool MatchEntityQuery(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(ref) const FEntityQuery& Query);

	/**
	 * Gets an array of all entity handles that currently match the given query.
	 * Useful for iterating over entities with specific components in Blueprints.
	 * WARNING: Iterating this array in Blueprint will be less performant than a native C++ Mass Processor, especially for large numbers of entities.
	 * @param WorldContextObject The world context.
	 * @param Query The query describing the required components.
	 * @return An array of matching entity handles.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mass|Query", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Matching Entities", Keywords = "get query filter mass entity entities array"))
	static TArray<FEntityHandle> GetMatchingEntities(const UObject* WorldContextObject, UPARAM(ref) const FEntityQuery& Query);


private:

	// Generic and Thunk implementations for GetFragment
	static void Generic_GetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, void* OutFragmentPtr, bool& bSuccess);
	DECLARE_FUNCTION(execGetFragment);

	// Generic and Thunk implementations for SetFragment
	static void Generic_SetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const void* InFragmentPtr, bool& bSuccess);
	DECLARE_FUNCTION(execSetFragment);

	// Generic and Thunk implementations for GetSharedFragment
	static void Generic_GetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, void* OutFragmentPtr, bool& bSuccess);
	DECLARE_FUNCTION(execGetSharedFragment);

	// Generic and Thunk implementations for SetSharedFragment
	static void Generic_SetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const void* InFragmentPtr, bool& bSuccess);
	DECLARE_FUNCTION(execSetSharedFragment);

	// Generic and Thunk implementations for GetConstSharedFragment
	static void Generic_GetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, void* OutFragmentPtr, bool& bSuccess);
	DECLARE_FUNCTION(execGetConstSharedFragment);

	// Generic and Thunk implementations for SetConstSharedFragment
	static void Generic_SetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, const void* InFragmentPtr, bool& bSuccess);
	DECLARE_FUNCTION(execSetConstSharedFragment);

	// Generic and Thunk implementations for GetFragmentFromTemplate
	static void Generic_GetFragmentFromTemplate(const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, void* OutFragmentPtr, bool& bSuccess);
	DECLARE_FUNCTION(execGetFragmentFromTemplate);

	// Generic and Thunk implementations for SetFragmentInTemplate
	static void Generic_SetFragmentInTemplate(FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const void* InFragmentPtr);
	DECLARE_FUNCTION(execSetFragmentInTemplate);

	// Generic and Thunk implementations for SetSharedFragmentInTemplate
	static void Generic_SetSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const void* InFragmentPtr);
	DECLARE_FUNCTION(execSetSharedFragmentInTemplate);

	// Generic and Thunk implementations for SetConstSharedFragmentInTemplate
	static void Generic_SetConstSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const void* InFragmentPtr);
	DECLARE_FUNCTION(execSetConstSharedFragmentInTemplate);

};

