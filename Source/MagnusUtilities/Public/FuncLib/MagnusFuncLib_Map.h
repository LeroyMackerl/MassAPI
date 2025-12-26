#pragma once

#include "CoreMinimal.h"
#include "UObject/Script.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UnrealType.h"
#include "UObject/ScriptMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "MagnusFuncLib_Map.generated.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

UENUM()
enum class EMapTreeContainerType : uint8
{
	NoContainer   UMETA(DisplayName = "None"),
	Array         UMETA(DisplayName = "Array"),
	Map           UMETA(DisplayName = "Map"),
	Set           UMETA(DisplayName = "Set")
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

UCLASS()
class MAGNUSUTILITIES_API UMagnusNodesFuncLib_MapHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	//———————— GetPair																								————
	
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(DisplayName = "Get Pair", CompactNodeTitle = "GET PAIR", MapParam = "TargetMap", MapKeyParam = "Key", MapValueParam = "Value"), Category = "Utilities|Map")
	static bool Map_GetPair(const TMap<int32, int32>& TargetMap, int32 Index, int32& Key, int32& Value);

	DECLARE_FUNCTION(execMap_GetPair);

	static bool GenericMap_GetPair(const void* TargetMap, const FMapProperty* MapProperty, int32 Index, void* OutKeyPtr, void* OutValuePtr);

	//———————— GetKey																								————

	UFUNCTION(BlueprintPure, CustomThunk, meta=(DisplayName = "Get Key", CompactNodeTitle = "GET KEY", MapParam = "TargetMap", MapKeyParam = "Key"), Category = "Utilities|Map")
	static bool Map_GetKey(const TMap<int32, int32>& TargetMap, int32 Index, int32& Key);

	DECLARE_FUNCTION(execMap_GetKey);

	static bool GenericMap_GetKey(const void* TargetMap, const FMapProperty* MapProperty, int32 Index, void* OutKeyPtr);
		
	//———————— GetValue																								————

	UFUNCTION(BlueprintPure, CustomThunk, meta=(DisplayName = "Get Value", CompactNodeTitle = "GET VALUE", MapParam = "TargetMap", MapValueParam = "Value"), Category = "Utilities|Map")
	static bool Map_GetValue(const TMap<int32, int32>& TargetMap, int32 Index, int32& Value);

	DECLARE_FUNCTION(execMap_GetValue);

	static bool GenericMap_GetValue(const void* TargetMap, const FMapProperty* MapProperty, int32 Index, void* OutValuePtr);
	
	//———————— GetKeys																								————

	UFUNCTION(BlueprintPure, CustomThunk, meta=(DisplayName = "Get Keys", CompactNodeTitle = "GET KEYS", MapParam = "TargetMap", MapKeyParam = "Keys", AutoCreateRefTerm = "Keys"), Category = "Utilities|Map")
	static void Map_Keys(const TMap<int32, int32>& TargetMap, TArray<int32>& Keys);
	
	DECLARE_FUNCTION(execMap_Keys);
	
	static void GenericMap_Keys(const void* MapAddr, const FMapProperty* MapProperty, const void* ArrayAddr, const FArrayProperty* ArrayProperty);

	//———————— GetValues																							————

	UFUNCTION(BlueprintPure, CustomThunk, meta=(DisplayName = "Get Values", CompactNodeTitle = "GET VALUES", MapParam = "TargetMap", MapValueParam = "Values", AutoCreateRefTerm = "Values"), Category = "Utilities|Map")
	static void Map_Values(const TMap<int32, int32>& TargetMap, TArray<int32>& Values);
	
	DECLARE_FUNCTION(execMap_Values);
	
	static void GenericMap_Values(const void* MapAddr, const FMapProperty* MapProperty, const void* ArrayAddr, const FArrayProperty* ArrayProperty);

	//———————— GetPair		  																						————

	UFUNCTION(BlueprintCallable, CustomThunk, BlueprintInternalUseOnly, Category="Magnus|Loop", meta=(MapParam = "TargetMap", MapKeyParam = "Key", MapValueParam = "Value"))
	static bool MagnusLoop_Map_GetPair(const TMap<int32, int32>& TargetMap, int32 Index, int32& Key, int32& Value);

	DECLARE_FUNCTION(execMagnusLoop_Map_GetPair);

	static bool GenericMagnusLoop_Map_GetPair(const void* TargetMap, const FMapProperty* MapProperty, int32 Index, void* OutKeyPtr, void* OutValuePtr);
	
	//———————— ContainsValue																						————

	UFUNCTION(BlueprintPure, CustomThunk, meta=(DisplayName = "Contains Value", CompactNodeTitle = "CONTAINS VALUE", MapParam = "TargetMap", MapValueParam = "Value", AutoCreateRefTerm = "Value", BlueprintThreadSafe), Category = "Utilities|Map")
	static bool Map_ContainsValue(const TMap<int32, int32>& TargetMap, const int32& Value);
	
	DECLARE_FUNCTION(execMap_ContainsValue);
	
	static bool GenericMap_FindValue(const void* TargetMap, const FMapProperty* MapProperty, const FProperty* ValueProperty, const void* ValuePtr);
	
	//———————— GetKeysFromValue																						————

	UFUNCTION(BlueprintPure, CustomThunk, meta=(DisplayName = "Get Keys From Value", CompactNodeTitle = "GET KEYS FROM VALUE", MapParam = "TargetMap", MapValueParam = "Value", MapKeyParam = "Keys", AutoCreateRefTerm = "Keys"), Category = "Utilities|Map")
	static void Map_KeysFromValue(const TMap<int32, int32>& TargetMap, int32 Value, TArray<int32>& Keys);
	
	DECLARE_FUNCTION(execMap_KeysFromValue);
	
	static void GenericMap_KeysFromValue(const void* MapAddr, const FMapProperty* MapProperty, const void* ArrayAddr, const FArrayProperty* ArrayProperty,  const FProperty* ValueProperty, const void* ValuePtr);

	//———————— RemoveEntries																						————
	
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(DisplayName = "Remove Entries", CompactNodeTitle = "REMOVE ENTRIES", MapParam = "TargetMap", MapKeyParam = "Keys",  AutoCreateRefTerm = "Keys"), Category = "Utilities|Map")
	static bool Map_RemoveEntries(const TMap<int32, int32>& TargetMap, const TArray<int32>& Keys);

	DECLARE_FUNCTION(execMap_RemoveEntries);

	static bool GenericMap_RemoveEntries(const void* MapAddr, const FMapProperty* MapProperty, const void* ArrayAddr, const FArrayProperty* ArrayProperty);
	
	//———————— RemoveEntriesWithValue																				————
	
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(DisplayName = "Remove Entries With Value", CompactNodeTitle = "REMOVE ENTRIES WITH VALUE", MapParam = "TargetMap", MapValueParam = "Value"), Category = "Utilities|Map")
	static bool Map_RemoveEntriesWithValue(const TMap<int32, int32>& TargetMap, const int32 Value);

	DECLARE_FUNCTION(execMap_RemoveEntriesWithValue);

	static bool GenericMap_RemoveEntriesWithValue(const void* MapAddr, const FMapProperty* MapProperty, const FProperty* ValueProperty, const void* ValuePtr);
	
	//———————— SetValueAt																							————
	
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(DisplayName = "Set Value At", MapParam = "TargetMap", MapValueParam = "Value"), Category = "Utilities|Map")
	static bool Map_SetValueAt(const TMap<int32, int32>& TargetMap, const int32 Index, const int32 Value);

	DECLARE_FUNCTION(execMap_SetValueAt);

	static bool GenericMap_SetValueAt(const void* MapAddr, const FMapProperty* MapProperty, const int32 Index, const void* ValuePtr);
	
	//———————— RandomItem																							————

	UFUNCTION(BlueprintPure, CustomThunk, meta=(DisplayName = "Random Map Item", CompactNodeTitle = "RANDOM MAP ITEM", MapParam = "TargetMap", MapKeyParam = "Key", MapValueParam = "Value"), Category = "Utilities|Map")
	static void Map_RandomItem(const TMap<int32, int32>& TargetMap, int32& Key, int32& Value);

	DECLARE_FUNCTION(execMap_RandomItem);

	static void GenericMap_RandomItem(const void* MapAddr, const FMapProperty* MapProperty, void* OutKeyPtr, void* OutValuePtr);
	
	//———————— RandomMapItemFromStream																				————
	
	UFUNCTION(BlueprintPure, CustomThunk, meta=(DisplayName = "Random Map Item From Stream", MapParam = "TargetMap", MapKeyParam = "Key", MapValueParam = "Value"), Category = "Utilities|Map")
	static void Map_RandomItemFromStream(const TMap<int32, int32>& TargetMap, UPARAM(Ref) FRandomStream& RandomStream, int32& Key, int32& Value);

	DECLARE_FUNCTION(execMap_RandomItemFromStream);

	static void GenericMap_RandomItemFromStream(const void* MapAddr, const FMapProperty* MapProperty, FRandomStream* RandomStream, void* OutKeyPtr, void* OutValuePtr);
	
	//———————— MapIdentical																							————
	
	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, CustomThunk, meta = (MapParam = "MapA", MapKeyParam = "KeysB", MapValueParam = "ValuesB"))
	static bool Map_Identical(const TMap<int32, int32>& MapA, const TArray<int32>& KeysB, const TArray<int32>& ValuesB);
	
	DECLARE_FUNCTION(execMap_Identical);
	
	static bool GenericMap_Identical(const void* MapAddr, const FMapProperty* MapProperty, const void* KeysBAddr, const FArrayProperty* KeysBProp, const void* ValuesBAddr, const FArrayProperty* ValuesBProp);
	
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

UCLASS()
class MAGNUSUTILITIES_API UMagnusFuncLib_MapTree : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	//================ Constant																					========

	static constexpr int32 MaxSupportedMapSize = TNumericLimits<int32>::Max();

	//================ ToolFunction																				========

	static FArrayProperty* FindArrayPropertyByName(const FStructProperty* StructProp,const FName& TargetName);
	static FSetProperty* FindSetPropertyByName(const FStructProperty* StructProp,const FName& TargetName);
	static FMapProperty* FindMapPropertyByName(const FStructProperty* StructProp,const FName& TargetName);

public:

	//================ MapTree																					========

	//———————— ArrayItem.Add																						————

	UFUNCTION(BlueprintCallable, /*BlueprintInternalUseOnly,*/ CustomThunk,
		meta=(Category = "Magnus|MapTree",DisplayName = "Add Array Item By Name",CompactNodeTitle = "ADD ITEM",MapParam = "TargetMap",MapKeyParam = "Key",CustomStructureParam = "Item",AutoCreateRefTerm = "Key, MemberName, Item"))
	static void MapTree_AddArrayItem(const TMap<int32, int32>& TargetMap, const int32& Key, const FName& MemberName, const int32& Item);
	DECLARE_FUNCTION(execMapTree_AddArrayItem);
	static void GenericMapTree_AddArrayItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* ItemPtr);

	//———————— ArrayItem.Remove																						————

	UFUNCTION(BlueprintCallable, /*BlueprintInternalUseOnly,*/ CustomThunk,
		meta=(Category = "Magnus|MapTree",DisplayName = "Remove Array Item By Name",CompactNodeTitle = "REMOVE ITEM",MapParam = "TargetMap",MapKeyParam = "Key",CustomStructureParam = "Item",AutoCreateRefTerm = "Key, MemberName, Item"))
	static void MapTree_RemoveArrayItem(const TMap<int32, int32>& TargetMap, const int32& Key, const FName& MemberName, const int32& Item);
	DECLARE_FUNCTION(execMapTree_RemoveArrayItem);
	static void GenericMapTree_RemoveArrayItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* ItemPtr);

	//———————— ArrayItem.Contains																					————

	UFUNCTION(BlueprintPure, /*BlueprintInternalUseOnly,*/ CustomThunk,
		meta=(Category = "Magnus|MapTree",DisplayName = "Contains Array Item By Name",CompactNodeTitle = "CONTAINS",MapParam = "TargetMap",MapKeyParam = "Key",CustomStructureParam = "Item",AutoCreateRefTerm = "Key, MemberName, Item"))
	static bool MapTree_ContainsArrayItem(const TMap<int32, int32>& TargetMap, const int32& Key, const FName& MemberName, const int32& Item);
	DECLARE_FUNCTION(execMapTree_ContainsArrayItem);
	static bool GenericMapTree_ContainsArrayItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* ItemPtr);

	//———————— SetItem.Add																							————

	UFUNCTION(BlueprintCallable, /*BlueprintInternalUseOnly,*/ CustomThunk,
		meta=(Category = "Magnus|MapTree",DisplayName = "Add Set Item By Name",CompactNodeTitle = "ADD ITEM",MapParam = "TargetMap",MapKeyParam = "Key",CustomStructureParam = "Item",AutoCreateRefTerm = "Key, MemberName, Item"))
	static void MapTree_AddSetItem(const TMap<int32, int32>& TargetMap, const int32& Key, const FName& MemberName, const int32& Item);
	DECLARE_FUNCTION(execMapTree_AddSetItem);
	static void GenericMapTree_AddSetItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* ItemPtr);

	//———————— SetItem.Remove																						————

	UFUNCTION(BlueprintCallable, /*BlueprintInternalUseOnly,*/ CustomThunk,
		meta=(Category = "Magnus|MapTree",DisplayName = "Remove Set Item By Name",CompactNodeTitle = "REMOVE ITEM",MapParam = "TargetMap",MapKeyParam = "Key",CustomStructureParam = "Item",AutoCreateRefTerm = "Key, MemberName, Item"))
	static void MapTree_RemoveSetItem(const TMap<int32, int32>& TargetMap, const int32& Key, const FName& MemberName, const int32& Item);
	DECLARE_FUNCTION(execMapTree_RemoveSetItem);
	static void GenericMapTree_RemoveSetItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* ItemPtr);

	//———————— SetItem.Contains																						————

	UFUNCTION(BlueprintPure, /*BlueprintInternalUseOnly,*/ CustomThunk,
		meta=(Category = "Magnus|MapTree",DisplayName = "Contains Set Item By Name",CompactNodeTitle = "CONTAINS",MapParam = "TargetMap",MapKeyParam = "Key",CustomStructureParam = "Item",AutoCreateRefTerm = "Key, MemberName, Item"))
	static bool MapTree_ContainsSetItem(const TMap<int32, int32>& TargetMap, const int32& Key, const FName& MemberName, const int32& Item);
	DECLARE_FUNCTION(execMapTree_ContainsSetItem);
	static bool GenericMapTree_ContainsSetItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* ItemPtr);

	//———————— MapItem.Add																							————

	UFUNCTION(BlueprintCallable, /*BlueprintInternalUseOnly,*/ CustomThunk,
		meta=(Category = "Magnus|MapTree",DisplayName = "Add Map Item By Name",CompactNodeTitle = "ADD ITEM",MapParam = "TargetMap",MapKeyParam = "Key",CustomStructureParam = "SubKey,Value",AutoCreateRefTerm = "Key, MemberName, SubKey, Value"))
	static void MapTree_AddMapItem(const TMap<int32, int32>& TargetMap, const int32& Key, const FName& MemberName, const int32& SubKey, const int32& Value);
	DECLARE_FUNCTION(execMapTree_AddMapItem);
	static void GenericMapTree_AddMapItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* SubKeyPtr, const void* ValuePtr);

	//———————— MapItem.Remove																						————

	UFUNCTION(BlueprintCallable, /*BlueprintInternalUseOnly,*/ CustomThunk,
		meta=(Category = "Magnus|MapTree",DisplayName = "Remove Map Item By Name",CompactNodeTitle = "REMOVE ITEM",MapParam = "TargetMap",MapKeyParam = "Key",CustomStructureParam = "SubKey",AutoCreateRefTerm = "Key, MemberName, SubKey"))
	static void MapTree_RemoveMapItem(const TMap<int32, int32>& TargetMap, const int32& Key, const FName& MemberName, const int32& SubKey);
	DECLARE_FUNCTION(execMapTree_RemoveMapItem);
	static void GenericMapTree_RemoveMapItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* SubKeyPtr);

	//———————— MapItem.Contains																						————

	UFUNCTION(BlueprintPure, /*BlueprintInternalUseOnly,*/ CustomThunk,
		meta=(Category = "Magnus|MapTree",DisplayName = "Contains Map Item By Name",CompactNodeTitle = "CONTAINS",MapParam = "TargetMap",MapKeyParam = "Key",CustomStructureParam = "SubKey,Value",AutoCreateRefTerm = "Key, MemberName, SubKey, Value"))
	static bool MapTree_ContainsMapItem(const TMap<int32, int32>& TargetMap, const int32& Key, const FName& MemberName, const int32& SubKey, const int32& Value);
	DECLARE_FUNCTION(execMapTree_ContainsMapItem);
	static bool GenericMapTree_ContainsMapItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* SubKeyPtr, const void* ValuePtr);
	
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
