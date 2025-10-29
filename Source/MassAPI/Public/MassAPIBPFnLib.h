#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MassAPISubsystem.h"
#include "MassAPIEnums.h" // (新) 包含新的枚举头文件
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
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "IsValid (EntityHandle)", Keywords = "valid isvalid mass entity"))
	static bool IsValid(const UObject* WorldContextObject, const FEntityHandle& EntityHandle);

	/**
	 * Checks if an entity has a specific fragment.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Has Mass Fragment", Keywords = "has hasfragment mass entity fragment"))
	static bool HasFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassFragment")) UScriptStruct* FragmentType);

	/**
	 * Checks if an entity has a specific tag.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Has Mass Tag", Keywords = "has hastag mass entity tag"))
	static bool HasTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassTag")) UScriptStruct* TagType);

	/**
	 * Gets a copy of a fragment's data from an entity.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, CustomThunk, BlueprintInternalUseOnly, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Mass Fragment", CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment", Keywords = "get getfragment gf mass entity fragment"))
	static void GetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);

	/**
	 * Sets the data for an existing fragment on an entity, or adds it if it doesn't exist.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Mass Fragment", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment", Keywords = "set setfragment sf mass entity fragment"))
	static void SetFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassFragment")) UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess);

	/**
	 * Removes a standard fragment from an entity. Does nothing if the fragment doesn't exist.
	 * @param WorldContextObject The world context.
	 * @param EntityHandle The entity to remove the fragment from.
	 * @param FragmentType The type of the fragment to remove.
	 * @return True if the fragment was removed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Remove Mass Fragment", Keywords = "remove delete fragment mass entity"))
	static bool RemoveFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassFragment")) UScriptStruct* FragmentType);

	/**
	 * Removes a shared fragment from an entity. Does nothing if the fragment doesn't exist.
	 * @param WorldContextObject The world context.
	 * @param EntityHandle The entity to remove the shared fragment from.
	 * @param FragmentType The type of the shared fragment to remove.
	 * @return True if the shared fragment was removed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Remove Mass Shared Fragment", Keywords = "remove delete shared fragment mass entity"))
	static bool RemoveSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassSharedFragment")) UScriptStruct* FragmentType);

	/**
	 * Removes a const shared fragment from an entity. Does nothing if the fragment doesn't exist.
	 * @param WorldContextObject The world context.
	 * @param EntityHandle The entity to remove the const shared fragment from.
	 * @param FragmentType The type of the const shared fragment to remove.
	 * @return True if the const shared fragment was removed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Remove Mass Const Shared Fragment", Keywords = "remove delete const shared fragment mass entity"))
	static bool RemoveConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassConstSharedFragment")) UScriptStruct* FragmentType);

	/**
	 * Adds a tag to an entity. Does nothing if the tag already exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Add Mass Tag", Keywords = "add tag mass entity"))
	static void AddTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassTag")) UScriptStruct* TagType);

	/**
	 * Removes a tag from an entity. Does nothing if the tag doesn't exist.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Remove Mass Tag", Keywords = "remove delete tag mass entity"))
	static void RemoveTag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassTag")) UScriptStruct* TagType);

	/**
	 * Synchronously destroys an entity.
	 * @param WorldContextObject The world context.
	 * @param EntityHandle The handle to the entity to destroy.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Destroy Entity", Keywords = "destroy delete remove mass entity"))
	static void DestroyEntity(const UObject* WorldContextObject, const FEntityHandle& EntityHandle);

	//----------------------------------------------------------------------//
	// Batch Entity Operations
	//----------------------------------------------------------------------//

	/**
	 * Synchronously destroys multiple entities.
	 * @param WorldContextObject The world context.
	 * @param EntityHandles An array of entity handles to destroy.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Destroy Entities", Keywords = "destroy delete remove batch mass entity"))
	static void DestroyEntities(const UObject* WorldContextObject, const TArray<FEntityHandle>& EntityHandles);

	// Batch Add/Remove Tag removed as requested

	//----------------------------------------------------------------------//
	// Shared Fragment Operations
	//----------------------------------------------------------------------//

	/**
	 * Gets a copy of a shared fragment's data from an entity.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, CustomThunk, BlueprintInternalUseOnly, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Shared Mass Fragment", CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment", Keywords = "get shared mass entity fragment"))
	static void GetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);

	/**
	 * Sets the shared fragment for an entity. This might change the entity's archetype.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Shared Mass Fragment", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment", Keywords = "set shared mass entity fragment"))
	static void SetSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(meta = (StructBase = "MassSharedFragment")) UScriptStruct* FragmentType, const FGenericStruct& InFragment, bool& bSuccess);

	//----------------------------------------------------------------------//
	// Const Shared Fragment Operations
	//----------------------------------------------------------------------//

	/**
	 * Gets a copy of a const shared fragment's data from an entity.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, CustomThunk, BlueprintInternalUseOnly, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Const Shared Mass Fragment", CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment", Keywords = "get const shared mass entity fragment"))
	static void GetConstSharedFragment(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);

	/**
	 * Sets the const shared fragment for an entity. This might change the entity's archetype.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Const Shared Mass Fragment", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment", Keywords = "set const shared mass entity fragment"))
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
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Build Entity From Template Data"))
	static FEntityHandle BuildEntityFromTemplateData(const UObject* WorldContextObject, const FEntityTemplateData& TemplateData);

	/**
	 * Synchronously builds multiple entities from the given template data.
	 * @param WorldContextObject The world context.
	 * @param Quantity The number of entities to build.
	 * @param TemplateData The template data to use for building the entities.
	 * @return An array of handles to the newly created entities.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Entity", meta = (WorldContext = "WorldContextObject", DisplayName = "Build Entities From Template Data"))
	static TArray<FEntityHandle> BuildEntitiesFromTemplateData(const UObject* WorldContextObject, int32 Quantity, const FEntityTemplateData& TemplateData);


	//----------------------------------------------------------------------//
	// TemplateData Operations
	//----------------------------------------------------------------------//

	/**
	 * Converts an FEntityTemplate into a modifiable FEntityTemplateData handle.
	 * This creates a new template data instance that can be edited with other nodes.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Template", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Template Data"))
	static FEntityTemplateData GetTemplateData(const UObject* WorldContextObject, const FEntityTemplate& Template);

	/**
	 * AUTO-CAST conversion function from FEntityTemplate to FEntityTemplateData.
	 * This enables automatic conversion in Blueprint graphs when connecting pins.
	 * Acts as an auto-cast node that implicitly converts template definitions to template data.
	 * @param WorldContextObject The world context.
	 * @param Template The template definition to convert.
	 * @return The constructed template data.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Template", meta = (
		WorldContext = "WorldContextObject",
		DisplayName = "Template To Template Data",
		CompactNodeTitle = "->",
		BlueprintAutocast,
		Keywords = "convert cast template data mass auto"))
	static FEntityTemplateData Conv_TemplateToTemplateData(const UObject* WorldContextObject, const FEntityTemplate& Template);

	/**
	 * Checks if the template data is empty (contains no fragments or tags).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Template", meta = (DisplayName = "IsEmpty (TemplateData)"))
	static bool IsEmpty_TemplateData(const FEntityTemplateData& TemplateData);

	/**
	 * Checks if the template data contains a specific fragment type.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Template", meta = (DisplayName = "Has Fragment (TemplateData)"))
	static bool HasFragment_TemplateData(const FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassFragment")) UScriptStruct* FragmentType);

	/**
	 * Adds a tag to the template data.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Template", meta = (DisplayName = "Add Tag (TemplateData)"))
	static void AddTag_TemplateData(UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassTag")) UScriptStruct* TagType);

	/**
	 * Removes a tag from the template data.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Template", meta = (DisplayName = "Remove Tag (TemplateData)"))
	static void RemoveTag_TemplateData(UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassTag")) UScriptStruct* TagType);

	/**
	 * Checks if the template data contains a specific tag type.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Template", meta = (DisplayName = "Has Tag (TemplateData)"))
	static bool HasTag_TemplateData(const FEntityTemplateData& TemplateData, UScriptStruct* TagType);

	/**
	 * Gets a copy of a fragment's initial value from the template data.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, CustomThunk, BlueprintInternalUseOnly, Category = "MassAPI|Template", meta = (DisplayName = "Get Fragment from Template", CustomStructureParam = "OutFragment", AutoCreateRefTerm = "OutFragment"))
	static void GetFragmentFromTemplate(const FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, FGenericStruct& OutFragment, bool& bSuccess);

	/**
	 * Adds or sets a fragment's initial value in the template data.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "MassAPI|Template", meta = (DisplayName = "Set Fragment in Template", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment"))
	static void SetFragmentInTemplate(UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassFragment")) UScriptStruct* FragmentType, const FGenericStruct& InFragment);

	/**
	 * Removes a fragment from the template data.
	 * @param TemplateData The template data to modify.
	 * @param FragmentType The type of the fragment to remove.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Template", meta = (DisplayName = "Remove Fragment in Template"))
	static void RemoveFragmentInTemplate(UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassFragment")) UScriptStruct* FragmentType);

	/**
	 * Adds or sets a shared fragment's value in the template data.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "MassAPI|Template", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Shared Fragment in Template", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment"))
	static void SetSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassSharedFragment")) UScriptStruct* FragmentType, const FGenericStruct& InFragment);

	/**
	 * Removes a shared fragment from the template data.
	 * @param WorldContextObject The world context.
	 * @param TemplateData The template data to modify.
	 * @param FragmentType The type of the shared fragment to remove.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Template", meta = (WorldContext = "WorldContextObject", DisplayName = "Remove Shared Fragment in Template"))
	static void RemoveSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassSharedFragment")) UScriptStruct* FragmentType);

	/**
	 * Adds or sets a const shared fragment's value in the template data.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category = "MassAPI|Template", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Const Shared Fragment in Template", CustomStructureParam = "InFragment", AutoCreateRefTerm = "InFragment"))
	static void SetConstSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassConstSharedFragment")) UScriptStruct* FragmentType, const FGenericStruct& InFragment);

	/**
	 * Removes a const shared fragment from the template data.
	 * @param WorldContextObject The world context.
	 * @param TemplateData The template data to modify.
	 * @param FragmentType The type of the const shared fragment to remove.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Template", meta = (WorldContext = "WorldContextObject", DisplayName = "Remove Const Shared Fragment in Template"))
	static void RemoveConstSharedFragmentInTemplate(const UObject* WorldContextObject, UPARAM(ref) FEntityTemplateData& TemplateData, UPARAM(meta = (StructBase = "MassConstSharedFragment")) UScriptStruct* FragmentType);


	//----------------------------------------------------------------------//
	// (新) Flag Fragment Operations (TemplateData)
	//----------------------------------------------------------------------//

	/**
	 * (新) 获取模板数据中 FEntityFlagFragment 的 64 位标志位掩码。
	 * @param TemplateData The template data to check.
	 * @return The 64-bit flag mask. Returns 0 if the template is invalid or has no flag fragment.
	 */
	 //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Template|Flags", meta = (DisplayName = "Get Template Flags (Bitmask)"))
	static int64 GetTemplateFlags(const FEntityTemplateData& TemplateData);

	/**
	 * (新) 检查模板数据是否拥有某个特定标志。
	 * @param TemplateData The template data to check.
	 * @param FlagToTest The flag to check for.
	 * @return True if the template has the FEntityFlagFragment and the flag is set, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Template|Flags", meta = (DisplayName = "Has Template Flag", CompactNodeTitle = "HAS"))
	static bool HasTemplateFlag(const FEntityTemplateData& TemplateData, EEntityFlags FlagToTest);

	/**
	 * (新) 向模板数据添加一个标志。
	 * (如果 FEntityFlagFragment 不存在，会自动添加)
	 * @param TemplateData The template data to modify.
	 * @param FlagToSet The flag to add.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Template|Flags", meta = (DisplayName = "Set Template Flag", CompactNodeTitle = "SET"))
	static void SetTemplateFlag(UPARAM(ref) FEntityTemplateData& TemplateData, EEntityFlags FlagToSet);

	/**
	 * (新) 从模板数据移除一个标志。
	 * @param TemplateData The template data to modify.
	 * @param FlagToClear The flag to remove.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Template|Flags", meta = (DisplayName = "Clear Template Flag", CompactNodeTitle = "CLEAR"))
	static void ClearTemplateFlag(UPARAM(ref) FEntityTemplateData& TemplateData, EEntityFlags FlagToClear);
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
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Query", meta = (WorldContext = "WorldContextObject", DisplayName = "Match Entity Query", Keywords = "match query filter mass entity"))
	static bool MatchEntityQuery(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, UPARAM(ref) const FEntityQuery& Query);

	/**
	 * Gets an array of all entity handles that currently match the given query.
	 * Useful for iterating over entities with specific components in Blueprints.
	 * WARNING: Iterating this array in Blueprint will be less performant than a native C++ Mass Processor, especially for large numbers of entities.
	 * @param WorldContextObject The world context.
	 * @param Query The query describing the required components.
	 * @return An array of matching entity handles.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Query", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Matching Entities", Keywords = "get query filter mass entity entities array"))
	static TArray<FEntityHandle> GetMatchingEntities(const UObject* WorldContextObject, UPARAM(ref) const FEntityQuery& Query);


	//----------------------------------------------------------------------//
	// (新) Flag Fragment Operations (已重构)
	//----------------------------------------------------------------------//

	/**
	 * 获取实体当前的 64 位标志位掩码。
	 * @param WorldContextObject The world context.
	 * @param EntityHandle The entity to check.
	 * @return The 64-bit flag mask. Returns 0 if the entity is invalid or has no flag fragment.
	 */
	 //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Flags", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Entity Flags (Bitmask)"))
	static int64 GetEntityFlags(const UObject* WorldContextObject, const FEntityHandle& EntityHandle);

	/**
	 * 检查实体是否拥有某个特定标志。
	 * @param WorldContextObject The world context.
	 * @param EntityHandle The entity to check.
	 * @param FlagToTest The flag to check for.
	 * @return True if the entity has the FEntityFlagFragment and the flag is set, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Flags", meta = (WorldContext = "WorldContextObject", DisplayName = "Has Entity Flag", CompactNodeTitle = "HAS"))
	static bool HasEntityFlag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToTest);

	/**
	 * 向实体添加一个标志。
	 * (如果 FEntityFlagFragment 不存在，会自动添加)
	 * @param WorldContextObject The world context.
	 * @param EntityHandle The entity to modify.
	 * @param FlagToSet The flag to add.
	 * @return True if the flag was successfully set (or was already set), false on failure (e.g., invalid entity).
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Flags", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Entity Flag", CompactNodeTitle = "SET"))
	static bool SetEntityFlag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToSet);

	/**
	 * 从实体移除一个标志。
	 * (如果 FEntityFlagFragment 不存在，此操作无效)
	 * @param WorldContextObject The world context.
	 * @param EntityHandle The entity to modify.
	 * @param FlagToClear The flag to remove.
	 * @return True if the flag was successfully cleared (or was already clear), false on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "MassAPI|Flags", meta = (WorldContext = "WorldContextObject", DisplayName = "Clear Entity Flag", CompactNodeTitle = "CLEAR"))
	static bool ClearEntityFlag(const UObject* WorldContextObject, const FEntityHandle& EntityHandle, EEntityFlags FlagToClear);

	/** 将一个 EEntityFlags 数组转换为一个 int64 位掩码。 (此函数保持不变) */
	//UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MassAPI|Flags", meta = (DisplayName = "Convert Flags Array to Bitmask", CompactNodeTitle = "-> Bitmask"))
	static int64 ConvertFlagsArrayToBitmask(const TArray<EEntityFlags>& Flags);


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

	// Generic implementation for RemoveFragmentInTemplate (No Thunk needed)
	static void Generic_RemoveFragmentInTemplate(FEntityTemplateData& TemplateData, UScriptStruct* FragmentType);

	// Generic implementation for RemoveSharedFragmentInTemplate (No Thunk needed)
	static void Generic_RemoveSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType);

	// Generic implementation for RemoveConstSharedFragmentInTemplate (No Thunk needed)
	static void Generic_RemoveConstSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType);

	// Generic and Thunk implementations for SetSharedFragmentInTemplate
	static void Generic_SetSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const void* InFragmentPtr);
	DECLARE_FUNCTION(execSetSharedFragmentInTemplate);

	// Generic and Thunk implementations for SetConstSharedFragmentInTemplate
	static void Generic_SetConstSharedFragmentInTemplate(const UObject* WorldContextObject, FEntityTemplateData& TemplateData, UScriptStruct* FragmentType, const void* InFragmentPtr);
	DECLARE_FUNCTION(execSetConstSharedFragmentInTemplate);

};
