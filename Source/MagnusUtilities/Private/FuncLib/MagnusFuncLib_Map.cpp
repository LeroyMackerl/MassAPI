#include "FuncLib/MagnusFuncLib_Map.h"
#include "Kismet/KismetArrayLibrary.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//———————— GetPair																							        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_GetPair)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(NULL);
	void* MapAddr = Stack.MostRecentPropertyAddress;
	FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	// 获取 Index 参数
	int32 Index = 0;
	Stack.StepCompiledIn<FIntProperty>(&Index);

	// 获取 Key 属性
	const FProperty* KeyProp = MapProperty->KeyProp;
	void* KeyStorageSpace = FMemory_Alloca(KeyProp->GetElementSize() * KeyProp->ArrayDim);
	KeyProp->InitializeValue(KeyStorageSpace);

	// 设置 Key 输出参数的栈位置
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	void* KeyPtr = Stack.MostRecentPropertyAddress;

	// 获取 Value 属性
	const FProperty* ValueProp = MapProperty->ValueProp;
	void* ValueStorageSpace = FMemory_Alloca(ValueProp->GetElementSize() * ValueProp->ArrayDim);
	ValueProp->InitializeValue(ValueStorageSpace);

	// 设置 Value 输出参数的栈位置
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericMap_GetPair(MapAddr, MapProperty, Index, KeyPtr, ValuePtr);
	P_NATIVE_END;

	// 清理临时分配的内存
	KeyProp->DestroyValue(KeyStorageSpace);
	ValueProp->DestroyValue(ValueStorageSpace);
}

bool UMagnusNodesFuncLib_MapHelper::GenericMap_GetPair(const void* TargetMap, const FMapProperty* MapProperty, int32 Index, void* OutKeyPtr, void* OutValuePtr)
{
	if (!TargetMap || !MapProperty || !OutKeyPtr || !OutValuePtr)
	{
		return false;
	}

	FScriptMapHelper MapHelper(MapProperty, TargetMap);
	if (Index < 0 || Index >= MapHelper.Num())
	{
		return false;
	}

	int32 ActualIndex = 0;
	for (int32 SparseIndex = 0; SparseIndex < MapHelper.GetMaxIndex(); ++SparseIndex)
	{
		if (MapHelper.IsValidIndex(SparseIndex))
		{
			if (ActualIndex == Index)
			{
				// 复制键
				const uint8* KeyPtr = MapHelper.GetKeyPtr(SparseIndex);
				MapProperty->KeyProp->CopyCompleteValue(OutKeyPtr, KeyPtr);

				// 复制值
				const uint8* ValuePtr = MapHelper.GetValuePtr(SparseIndex);
				MapProperty->ValueProp->CopyCompleteValue(OutValuePtr, ValuePtr);

				return true;
			}
			++ActualIndex;
		}
	}

	return false;
}

//———————— GetKey																							        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_GetKey)
{
	// Get target map
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	const void* MapAddr = Stack.MostRecentPropertyAddress;
	const FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}
	
	// Get index
	P_GET_PROPERTY(FIntProperty, Index);
	
	// Get key out value
	FProperty* KeyProp = MapProperty->KeyProp;
	const int32 KeyPropertySize = KeyProp->GetElementSize() * KeyProp->ArrayDim;
	void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
	KeyProp->InitializeValue(KeyStorageSpace);
	
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(KeyStorageSpace);
	
	const FFieldClass* KeyPropClass = KeyProp->GetClass();
	const FFieldClass* MostRecentPropClass = Stack.MostRecentProperty->GetClass();
	void* ItemPtr;
	if (Stack.MostRecentPropertyAddress != nullptr &&
		KeyPropertySize == Stack.MostRecentProperty->GetElementSize()*Stack.MostRecentProperty->ArrayDim &&
		(MostRecentPropClass->IsChildOf(KeyPropClass) || KeyPropClass->IsChildOf(MostRecentPropClass)))
	{
		ItemPtr = Stack.MostRecentPropertyAddress;
	}
	else
	{
		ItemPtr = KeyStorageSpace;
	}
	
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericMap_GetKey(MapAddr, MapProperty, Index, ItemPtr);
	P_NATIVE_END
	
	KeyProp->DestroyValue(KeyStorageSpace);
}

bool UMagnusNodesFuncLib_MapHelper::GenericMap_GetKey(const void* TargetMap, const FMapProperty* MapProperty, int32 Index, void* OutKeyPtr)
{
	if(TargetMap)
	{
		FScriptMapHelper MapHelper(MapProperty, TargetMap);
		if(Index < 0 || Index > MapHelper.Num()-1) return false;

		MapHelper.KeyProp->CopyCompleteValueFromScriptVM(OutKeyPtr, MapHelper.GetKeyPtr(MapHelper.FindInternalIndex(Index)));
		
		return true;
	}
	
	return false;
}

//———————— GetValue																							        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_GetValue)
{
	// Get target map
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	const void* MapAddr = Stack.MostRecentPropertyAddress;
	const FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}
	
	// Get index
	P_GET_PROPERTY(FIntProperty, Index);
	
	// Get value out value
	FProperty* ValueProp = MapProperty->ValueProp;
	const int32 ValuePropertySize = ValueProp->GetElementSize() * ValueProp->ArrayDim;
	void* ValueStorageSpace = FMemory_Alloca(ValuePropertySize);
	ValueProp->InitializeValue(ValueStorageSpace);
	
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(ValueStorageSpace);
	
	const FFieldClass* ValuePropClass = ValueProp->GetClass();
	const FFieldClass* MostRecentPropClass = Stack.MostRecentProperty->GetClass();
	
	void* ItemPtr;
	if (Stack.MostRecentPropertyAddress != nullptr &&
		ValuePropertySize == Stack.MostRecentProperty->GetElementSize()*Stack.MostRecentProperty->ArrayDim &&
		(MostRecentPropClass->IsChildOf(ValuePropClass) || ValuePropClass->IsChildOf(MostRecentPropClass)))
	{
		ItemPtr = Stack.MostRecentPropertyAddress;
	}
	else
	{
		ItemPtr = ValueStorageSpace;
	}
	
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericMap_GetValue(MapAddr, MapProperty, Index, ItemPtr);
	P_NATIVE_END

	ValueProp->DestroyValue(ValueStorageSpace);
}

bool UMagnusNodesFuncLib_MapHelper::GenericMap_GetValue(const void* TargetMap, const FMapProperty* MapProperty, int32 Index, void* OutValuePtr)
{
	if(TargetMap)
	{
		FScriptMapHelper MapHelper(MapProperty, TargetMap);
		if(Index < 0 || Index > MapHelper.Num()-1) return false;
			
		MapHelper.ValueProp->CopySingleValueToScriptVM(OutValuePtr, MapHelper.GetValuePtr(MapHelper.FindInternalIndex(Index)));
		return true;
	}
	
	return false;
}

//———————— GetKeys																							        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_Keys)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	const void* MapAddr = Stack.MostRecentPropertyAddress;
	const FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);
	const void* ArrayAddr = Stack.MostRecentPropertyAddress;
	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	if (!ArrayProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}
	
	P_FINISH;
	P_NATIVE_BEGIN;
	GenericMap_Keys(MapAddr, MapProperty, ArrayAddr, ArrayProperty);
	P_NATIVE_END
}

void UMagnusNodesFuncLib_MapHelper::GenericMap_Keys(const void* MapAddr, const FMapProperty* MapProperty, const void* ArrayAddr, const FArrayProperty* ArrayProperty)
{
	if(MapAddr && ArrayAddr && ensure(MapProperty->KeyProp->GetID() == ArrayProperty->Inner->GetID()) )
	{
		FScriptMapHelper MapHelper(MapProperty, MapAddr);
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayAddr);
		ArrayHelper.EmptyValues();

		const FProperty* InnerProp = ArrayProperty->Inner;

		int32 Size = MapHelper.Num();
		for( int32 I = 0; Size; ++I )
		{
			if(MapHelper.IsValidIndex(I))
			{
				const int32 LastIndex = ArrayHelper.AddValue();
				InnerProp->CopySingleValueToScriptVM(ArrayHelper.GetRawPtr(LastIndex), MapHelper.GetKeyPtr(I));
				--Size;
			}
		}
	}
}

//———————— GetValues																						        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_Values)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	const void* MapAddr = Stack.MostRecentPropertyAddress;
	const FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);
	const void* ArrayAddr = Stack.MostRecentPropertyAddress;
	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	if (!ArrayProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}
	
	P_FINISH;
	P_NATIVE_BEGIN;
	GenericMap_Values(MapAddr, MapProperty, ArrayAddr, ArrayProperty);
	P_NATIVE_END
}

void UMagnusNodesFuncLib_MapHelper::GenericMap_Values(const void* MapAddr, const FMapProperty* MapProperty, const void* ArrayAddr, const FArrayProperty* ArrayProperty)
{
	if(MapAddr && ArrayAddr && ensure(MapProperty->ValueProp->GetID() == ArrayProperty->Inner->GetID()))
	{
		FScriptMapHelper MapHelper(MapProperty, MapAddr);
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayAddr);
		ArrayHelper.EmptyValues();

		const FProperty* InnerProp = ArrayProperty->Inner;
		
		int32 Size = MapHelper.Num();
		for( int32 I = 0; Size; ++I )
		{
			if(MapHelper.IsValidIndex(I))
			{
				const int32 LastIndex = ArrayHelper.AddValue();
				InnerProp->CopySingleValueToScriptVM(ArrayHelper.GetRawPtr(LastIndex), MapHelper.GetValuePtr(I));
				--Size;
			}
		}
	}
}

//———————— GetPair		  																					        ————
	
DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMagnusLoop_Map_GetPair)
{
    // 获取Map参数
    Stack.MostRecentProperty = nullptr;
    Stack.StepCompiledIn<FMapProperty>(nullptr);
    void* MapPtr = Stack.MostRecentPropertyAddress;
    FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
    if (!MapProperty)
    {
        Stack.bArrayContextFailed = true;
        return;
    }

    // 获取索引参数
    P_GET_PROPERTY(FIntProperty, Index);

    // 获取Key输出参数
    Stack.MostRecentProperty = nullptr;
    Stack.StepCompiledIn<FProperty>(nullptr);
    void* KeyPtr = Stack.MostRecentPropertyAddress;
    FProperty* KeyProperty = Stack.MostRecentProperty;

    // 获取Value输出参数
    Stack.MostRecentProperty = nullptr;
    Stack.StepCompiledIn<FProperty>(nullptr);
    void* ValuePtr = Stack.MostRecentPropertyAddress;
    FProperty* ValueProperty = Stack.MostRecentProperty;

    P_FINISH;

    // 调用泛型函数执行实际操作
    bool bSuccess = GenericMagnusLoop_Map_GetPair(MapPtr, MapProperty, Index, KeyPtr, ValuePtr);
    *(bool*)RESULT_PARAM = bSuccess;
}

bool UMagnusNodesFuncLib_MapHelper::GenericMagnusLoop_Map_GetPair(const void* TargetMap, const FMapProperty* MapProperty, int32 Index, void* OutKeyPtr, void* OutValuePtr)
{
    if (!MapProperty || !TargetMap || !OutKeyPtr || !OutValuePtr)
    {
        return false;
    }

    // 使用FScriptMapHelper处理Map
    FScriptMapHelper MapHelper(MapProperty, TargetMap);
    
    if (Index < 0 || Index >= MapHelper.Num())
    {
        return false;
    }

    // 查找第Index个有效元素
    int32 ElementIndex = INDEX_NONE;
    int32 CurrentIndex = 0;
    
    for (int32 SparseIndex = 0; SparseIndex < MapHelper.GetMaxIndex(); ++SparseIndex)
    {
        if (MapHelper.IsValidIndex(SparseIndex))
        {
            if (CurrentIndex == Index)
            {
                ElementIndex = SparseIndex;
                break;
            }
            ++CurrentIndex;
        }
    }

    if (ElementIndex == INDEX_NONE)
    {
        return false;
    }

    // 复制Key和Value到输出参数
    MapProperty->KeyProp->CopyCompleteValue(OutKeyPtr, MapHelper.GetKeyPtr(ElementIndex));
    MapProperty->ValueProp->CopyCompleteValue(OutValuePtr, MapHelper.GetValuePtr(ElementIndex));

    return true;
}

//———————— ContainsValue																					        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_ContainsValue)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	const void* MapAddr = Stack.MostRecentPropertyAddress;
	const FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrValueProp = MapProperty->ValueProp;
	const int32 ValuePropertySize = CurrValueProp->GetElementSize() * CurrValueProp->ArrayDim;
	void* ValueStorageSpace = FMemory_Alloca(ValuePropertySize);
	CurrValueProp->InitializeValue(ValueStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(ValueStorageSpace);
	
	const FFieldClass* CurrValuePropClass = CurrValueProp->GetClass();
	const FFieldClass* MostRecentPropClass = Stack.MostRecentProperty->GetClass();
	void* ValuePtr;
	// If the destination and the inner type are identical in size and their field classes derive from one another, then permit the writing out of the array element to the destination memory
	if (Stack.MostRecentPropertyAddress != nullptr && (ValuePropertySize == Stack.MostRecentProperty->GetElementSize()*Stack.MostRecentProperty->ArrayDim) &&
		(MostRecentPropClass->IsChildOf(CurrValuePropClass) || CurrValuePropClass->IsChildOf(MostRecentPropClass)))
	{
		ValuePtr = Stack.MostRecentPropertyAddress;
	}
	else
	{
		ValuePtr = ValueStorageSpace;
	}

	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericMap_FindValue(MapAddr, MapProperty, CurrValueProp, ValuePtr);
	P_NATIVE_END;

	CurrValueProp->DestroyValue(ValueStorageSpace);
}

bool UMagnusNodesFuncLib_MapHelper::GenericMap_FindValue(const void* TargetMap, const FMapProperty* MapProperty, const FProperty* ValueProperty, const void* ValuePtr)
{
	if(TargetMap)
	{
		FScriptMapHelper MapHelper(MapProperty, TargetMap);
		for(int32 i = 0; i < MapHelper.Num(); i++)
		{
			const void* MapValuePtr = MapHelper.GetValuePtr(MapHelper.FindInternalIndex(i));
			if(!MapValuePtr) return false;
			if(ValueProperty->Identical(ValuePtr, MapValuePtr, PPF_None)) return true;
		}
	}
	
	return false;
}

//———————— GetKeysFromValue																					        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_KeysFromValue)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	const void* MapAddr = Stack.MostRecentPropertyAddress;
	const FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrValueProp = MapProperty->ValueProp;
	const int32 ValuePropertySize = CurrValueProp->GetElementSize() * CurrValueProp->ArrayDim;
	void* ValueStorageSpace = FMemory_Alloca(ValuePropertySize);
	CurrValueProp->InitializeValue(ValueStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(ValueStorageSpace);
	
	const FFieldClass* CurrValuePropClass = CurrValueProp->GetClass();
	const FFieldClass* MostRecentPropClass = Stack.MostRecentProperty->GetClass();
	void* ValuePtr;
	// If the destination and the inner type are identical in size and their field classes derive from one another, then permit the writing out of the array element to the destination memory
	if (Stack.MostRecentPropertyAddress != nullptr && (ValuePropertySize == Stack.MostRecentProperty->GetElementSize()*Stack.MostRecentProperty->ArrayDim) &&
		(MostRecentPropClass->IsChildOf(CurrValuePropClass) || CurrValuePropClass->IsChildOf(MostRecentPropClass)))
	{
		ValuePtr = Stack.MostRecentPropertyAddress;
	}
	else
	{
		ValuePtr = ValueStorageSpace;
	}
	
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);
	const void* ArrayAddr = Stack.MostRecentPropertyAddress;
	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	if (!ArrayProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}
	
	P_FINISH;
	P_NATIVE_BEGIN;
	GenericMap_KeysFromValue(MapAddr, MapProperty, ArrayAddr, ArrayProperty, CurrValueProp, ValuePtr);
	P_NATIVE_END

	CurrValueProp->DestroyValue(ValueStorageSpace);
}

void UMagnusNodesFuncLib_MapHelper::GenericMap_KeysFromValue(const void* MapAddr, const FMapProperty* MapProperty, const void* ArrayAddr, const FArrayProperty* ArrayProperty,  const FProperty* ValueProperty, const void* ValuePtr)
{
	if(MapAddr)
	{
		FScriptMapHelper MapHelper(MapProperty, MapAddr);
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayAddr);
		ArrayHelper.EmptyValues();

		const FProperty* InnerProp = ArrayProperty->Inner;
		
		for(int32 i = 0; i < MapHelper.Num(); i++)
		{
			const void* MapValuePtr = MapHelper.GetValuePtr(MapHelper.FindInternalIndex(i));
			if(!MapValuePtr) continue;
			
			if(ValueProperty->Identical(ValuePtr, MapValuePtr, PPF_None))
			{
				const int32 LastIndex = ArrayHelper.AddValue();
				InnerProp->CopySingleValueToScriptVM(ArrayHelper.GetRawPtr(LastIndex), MapHelper.GetKeyPtr(i));
			}
		}
	}
}

//———————— RemoveEntries																					        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_RemoveEntries)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	const void* MapAddr = Stack.MostRecentPropertyAddress;
	const FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);
	void* ArrayAddr = Stack.MostRecentPropertyAddress;
	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	if (!ArrayProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}
	
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericMap_RemoveEntries(MapAddr, MapProperty, ArrayAddr, ArrayProperty);
	P_NATIVE_END

	ArrayProperty->DestroyValue(ArrayAddr);
}

bool UMagnusNodesFuncLib_MapHelper::GenericMap_RemoveEntries(const void* MapAddr, const FMapProperty* MapProperty, const void* ArrayAddr, const FArrayProperty* ArrayProperty)
{
	if(MapAddr)
	{
		bool bResult = true;
		FScriptMapHelper MapHelper(MapProperty, MapAddr);
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayAddr);

		for(int32 i = 0; i < ArrayHelper.Num(); i++)
		{
			if(!MapHelper.RemovePair(ArrayHelper.GetRawPtr(i))) bResult = false;
		}

		return bResult;
	}
	
	return false;
}

//———————— RemoveEntriesWithValue																			        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_RemoveEntriesWithValue)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	const void* MapAddr = Stack.MostRecentPropertyAddress;
	const FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrValueProp = MapProperty->ValueProp;
	const int32 ValuePropertySize = CurrValueProp->GetElementSize() * CurrValueProp->ArrayDim;
	void* ValueStorageSpace = FMemory_Alloca(ValuePropertySize);
	CurrValueProp->InitializeValue(ValueStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(ValueStorageSpace);
	
	const FFieldClass* CurrValuePropClass = CurrValueProp->GetClass();
	const FFieldClass* MostRecentPropClass = Stack.MostRecentProperty->GetClass();
	void* ValuePtr;
	// If the destination and the inner type are identical in size and their field classes derive from one another, then permit the writing out of the array element to the destination memory
	if (Stack.MostRecentPropertyAddress != nullptr && (ValuePropertySize == Stack.MostRecentProperty->GetElementSize()*Stack.MostRecentProperty->ArrayDim) &&
		(MostRecentPropClass->IsChildOf(CurrValuePropClass) || CurrValuePropClass->IsChildOf(MostRecentPropClass)))
	{
		ValuePtr = Stack.MostRecentPropertyAddress;
	}
	else
	{
		ValuePtr = ValueStorageSpace;
	}
	
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericMap_RemoveEntriesWithValue(MapAddr, MapProperty, CurrValueProp, ValuePtr);
	P_NATIVE_END

	CurrValueProp->DestroyValue(ValueStorageSpace);
}

bool UMagnusNodesFuncLib_MapHelper::GenericMap_RemoveEntriesWithValue(const void* MapAddr, const FMapProperty* MapProperty, const FProperty* ValueProperty, const void* ValuePtr)
{
	if(MapAddr)
	{
		FScriptMapHelper MapHelper(MapProperty, MapAddr);

		bool bFound = false;
		TArray<int32> Indices;
		for(int32 i = MapHelper.Num()-1; i >= 0; i--)
		{
			const void* MapValuePtr = MapHelper.GetValuePtr(MapHelper.FindInternalIndex(i));
			if(!MapValuePtr) continue;
			
			if(ValueProperty->Identical(ValuePtr, MapValuePtr, PPF_None))
			{
				bFound = true;
				Indices.Add(i);
			}
		}

		for(int32 i = 0; i < Indices.Num(); i++)
		{
			MapHelper.RemoveAt(MapHelper.FindInternalIndex(Indices[i]));
		}

		return bFound;
	}

	return false;
}

//———————— SetValueAt																						        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_SetValueAt)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	const void* MapAddr = Stack.MostRecentPropertyAddress;
	const FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	P_GET_PROPERTY(FIntProperty, Index);
	
	const FProperty* CurrValueProp = MapProperty->ValueProp;
	const int32 ValuePropertySize = CurrValueProp->GetElementSize() * CurrValueProp->ArrayDim;
	void* ValueStorageSpace = FMemory_Alloca(ValuePropertySize);
	CurrValueProp->InitializeValue(ValueStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(ValueStorageSpace);
	
	const FFieldClass* CurrValuePropClass = CurrValueProp->GetClass();
	const FFieldClass* MostRecentPropClass = Stack.MostRecentProperty->GetClass();
	void* ValuePtr;
	// If the destination and the inner type are identical in size and their field classes derive from one another, then permit the writing out of the array element to the destination memory
	if (Stack.MostRecentPropertyAddress != nullptr && (ValuePropertySize == Stack.MostRecentProperty->GetElementSize()*Stack.MostRecentProperty->ArrayDim) &&
		(MostRecentPropClass->IsChildOf(CurrValuePropClass) || CurrValuePropClass->IsChildOf(MostRecentPropClass)))
	{
		ValuePtr = Stack.MostRecentPropertyAddress;
	}
	else
	{
		ValuePtr = ValueStorageSpace;
	}
	
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*) RESULT_PARAM = GenericMap_SetValueAt(MapAddr, MapProperty, Index, ValuePtr);
	P_NATIVE_END

	CurrValueProp->DestroyValue(ValueStorageSpace);
}

bool UMagnusNodesFuncLib_MapHelper::GenericMap_SetValueAt(const void* MapAddr, const FMapProperty* MapProperty, const int32 Index, const void* ValuePtr)
{
	if(MapAddr)
	{
		FScriptMapHelper MapHelper(MapProperty, MapAddr);
		
		if(Index < 0 || Index > MapHelper.Num()-1) return false;
		
		const FProperty* KeyProperty = MapProperty->KeyProp;
		const int32 KeyPropertySize = KeyProperty->GetElementSize() * KeyProperty->ArrayDim;
		void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
		KeyProperty->InitializeValue(KeyStorageSpace);
		
		MapHelper.KeyProp->CopyCompleteValueFromScriptVM(KeyStorageSpace, MapHelper.GetKeyPtr(MapHelper.FindInternalIndex(Index)));
		
		MapHelper.AddPair(KeyStorageSpace, ValuePtr);
		return true;
	}
	
	return false;
}

//———————— RandomItem																						        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_RandomItem)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	const void* MapAddr = Stack.MostRecentPropertyAddress;
	const FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}
	
	// Get key out value
	FProperty* KeyProp = MapProperty->KeyProp;
	const int32 KeyPropertySize = KeyProp->GetElementSize() * KeyProp->ArrayDim;
	void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
	KeyProp->InitializeValue(KeyStorageSpace);
	
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(KeyStorageSpace);
	
	const FFieldClass* KeyPropClass = KeyProp->GetClass();
	const FFieldClass* KeyMostRecentPropClass = Stack.MostRecentProperty->GetClass();
	void* KeyPtr;
	if (Stack.MostRecentPropertyAddress != nullptr &&
		KeyPropertySize == Stack.MostRecentProperty->GetElementSize()*Stack.MostRecentProperty->ArrayDim &&
		(KeyMostRecentPropClass->IsChildOf(KeyPropClass) || KeyPropClass->IsChildOf(KeyMostRecentPropClass)))
	{
		KeyPtr = Stack.MostRecentPropertyAddress;
	}
	else
	{
		KeyPtr = KeyStorageSpace;
	}
	
	// Get value out value
	FProperty* ValueProp = MapProperty->ValueProp;
	const int32 ValuePropertySize = ValueProp->GetElementSize() * ValueProp->ArrayDim;
	void* ValueStorageSpace = FMemory_Alloca(ValuePropertySize);
	ValueProp->InitializeValue(ValueStorageSpace);
	
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(ValueStorageSpace);
	
	const FFieldClass* ValuePropClass = ValueProp->GetClass();
	const FFieldClass* ValueMostRecentPropClass = Stack.MostRecentProperty->GetClass();
	
	void* ValuePtr;
	if (Stack.MostRecentPropertyAddress != nullptr &&
		ValuePropertySize == Stack.MostRecentProperty->GetElementSize()*Stack.MostRecentProperty->ArrayDim &&
		(ValueMostRecentPropClass->IsChildOf(ValuePropClass) || ValuePropClass->IsChildOf(ValueMostRecentPropClass)))
	{
		ValuePtr = Stack.MostRecentPropertyAddress;
	}
	else
	{
		ValuePtr = ValueStorageSpace;
	}

	P_FINISH;
	P_NATIVE_BEGIN;
	GenericMap_RandomItem(MapAddr, MapProperty, KeyPtr, ValuePtr);
	P_NATIVE_END
	
	KeyProp->DestroyValue(KeyStorageSpace);
	ValueProp->DestroyValue(ValueStorageSpace);
}

void UMagnusNodesFuncLib_MapHelper::GenericMap_RandomItem(const void* MapAddr, const FMapProperty* MapProperty, void* OutKeyPtr, void* OutValuePtr)
{
	if(MapAddr)
	{
		FScriptMapHelper MapHelper(MapProperty, MapAddr);
		const int32 Index = FMath::RandRange(0, MapHelper.Num() - 1);
		
		MapHelper.KeyProp->CopyCompleteValueFromScriptVM(OutKeyPtr, MapHelper.GetKeyPtr(MapHelper.FindInternalIndex(Index)));
		MapHelper.ValueProp->CopyCompleteValueFromScriptVM(OutValuePtr, MapHelper.GetValuePtr(MapHelper.FindInternalIndex(Index)));
	}
}

//———————— RandomMapItemFromStream																			        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_RandomItemFromStream)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	const void* MapAddr = Stack.MostRecentPropertyAddress;
	const FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}
	
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	FRandomStream* RandomStream = (FRandomStream*)Stack.MostRecentPropertyAddress;
	
	// Get key out value
	FProperty* KeyProp = MapProperty->KeyProp;
	const int32 KeyPropertySize = KeyProp->GetElementSize() * KeyProp->ArrayDim;
	void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
	KeyProp->InitializeValue(KeyStorageSpace);
	
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(KeyStorageSpace);
	
	const FFieldClass* KeyPropClass = KeyProp->GetClass();
	const FFieldClass* KeyMostRecentPropClass = Stack.MostRecentProperty->GetClass();
	void* KeyPtr;
	if (Stack.MostRecentPropertyAddress != nullptr &&
		KeyPropertySize == Stack.MostRecentProperty->GetElementSize()*Stack.MostRecentProperty->ArrayDim &&
		(KeyMostRecentPropClass->IsChildOf(KeyPropClass) || KeyPropClass->IsChildOf(KeyMostRecentPropClass)))
	{
		KeyPtr = Stack.MostRecentPropertyAddress;
	}
	else
	{
		KeyPtr = KeyStorageSpace;
	}
	
	// Get value out value
	FProperty* ValueProp = MapProperty->ValueProp;
	const int32 ValuePropertySize = ValueProp->GetElementSize() * ValueProp->ArrayDim;
	void* ValueStorageSpace = FMemory_Alloca(ValuePropertySize);
	ValueProp->InitializeValue(ValueStorageSpace);
	
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(ValueStorageSpace);
	
	const FFieldClass* ValuePropClass = ValueProp->GetClass();
	const FFieldClass* ValueMostRecentPropClass = Stack.MostRecentProperty->GetClass();
	
	void* ValuePtr;
	if (Stack.MostRecentPropertyAddress != nullptr &&
		ValuePropertySize == Stack.MostRecentProperty->GetElementSize()*Stack.MostRecentProperty->ArrayDim &&
		(ValueMostRecentPropClass->IsChildOf(ValuePropClass) || ValuePropClass->IsChildOf(ValueMostRecentPropClass)))
	{
		ValuePtr = Stack.MostRecentPropertyAddress;
	}
	else
	{
		ValuePtr = ValueStorageSpace;
	}

	P_FINISH;
	P_NATIVE_BEGIN;
	GenericMap_RandomItemFromStream(MapAddr, MapProperty, RandomStream, KeyPtr, ValuePtr);
	P_NATIVE_END
	
	KeyProp->DestroyValue(KeyStorageSpace);
	ValueProp->DestroyValue(ValueStorageSpace);
}

void UMagnusNodesFuncLib_MapHelper::GenericMap_RandomItemFromStream(const void* MapAddr, const FMapProperty* MapProperty, FRandomStream* RandomStream, void* OutKeyPtr, void* OutValuePtr)
{
	if(MapAddr)
	{
		FScriptMapHelper MapHelper(MapProperty, MapAddr);
		const int32 Index = RandomStream->RandRange(0, MapHelper.Num() - 1);
		
		MapHelper.KeyProp->CopyCompleteValueFromScriptVM(OutKeyPtr, MapHelper.GetKeyPtr(MapHelper.FindInternalIndex(Index)));
		MapHelper.ValueProp->CopyCompleteValueFromScriptVM(OutValuePtr, MapHelper.GetValuePtr(MapHelper.FindInternalIndex(Index)));
	}
}

//———————— MapIdentical																						        ————

DEFINE_FUNCTION(UMagnusNodesFuncLib_MapHelper::execMap_Identical)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	const void* MapAddr= Stack.MostRecentPropertyAddress;
	const FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	// Retrieve the first array
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(NULL);
	void* ArrayKeysAddr = Stack.MostRecentPropertyAddress;
	FArrayProperty* ArrayKeysProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	if (!ArrayKeysProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	// Retrieve the second array
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(NULL);
	void* ArrayValuesAddr = Stack.MostRecentPropertyAddress;
	FArrayProperty* ArrayValuesProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	if (!ArrayValuesProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}
	
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericMap_Identical(MapAddr, MapProperty, ArrayKeysAddr, ArrayKeysProperty, ArrayValuesAddr, ArrayValuesProperty);
	P_NATIVE_END;
}

bool UMagnusNodesFuncLib_MapHelper::GenericMap_Identical(const void* MapAddr, const FMapProperty* MapProperty, const void* KeysBAddr, const FArrayProperty* KeysBProp, const void* ValuesBAddr, const FArrayProperty* ValuesBProp)
{
	if(MapAddr && KeysBAddr && ValuesBAddr)
	{
		FScriptMapHelper MapHelper(MapProperty, MapAddr);
		FScriptArrayHelper KeysBHelper(KeysBProp, KeysBAddr);
		FScriptArrayHelper ValuesBHelper(ValuesBProp, ValuesBAddr);

		if(MapHelper.Num() == KeysBHelper.Num() && MapHelper.Num() == ValuesBHelper.Num())
		{
			for(int32 MapIndex = 0; MapIndex < MapHelper.Num(); MapIndex++)
			{
				bool bFound = false;
				const FProperty* KeyProperty = MapHelper.GetKeyProperty();
				const FProperty* ValueProperty = MapHelper.GetValueProperty();
				const int32 InternalIndex = MapHelper.FindInternalIndex(MapIndex);
				for(int32 KeyIndex = 0; KeyIndex < KeysBHelper.Num(); KeyIndex++)
				{
					if(KeyProperty->Identical(MapHelper.GetKeyPtr(InternalIndex), KeysBHelper.GetRawPtr(KeyIndex)) &&
						ValueProperty->Identical(MapHelper.GetValuePtr(InternalIndex), ValuesBHelper.GetRawPtr(KeyIndex)))
					{
						bFound = true;
					}
				}

				if(!bFound) return false;
			}
			
			return true;
		}
	}
	
	return false;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ ToolFunction																					========

FArrayProperty* UMagnusFuncLib_MapTree::FindArrayPropertyByName(const FStructProperty* StructProp, const FName& TargetName)
{
	if (!StructProp || TargetName.IsNone())
	{
		return nullptr;
	}

	const FString TargetNameStr = TargetName.ToString();
	FArrayProperty* FoundArrayProp = nullptr;

	// 遍历结构体的所有属性
	for (TFieldIterator<FProperty> PropIt(StructProp->Struct); PropIt; ++PropIt)
	{
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(*PropIt);
		if (!ArrayProp)
		{
			continue;
		}

		// 方法1: 检查 DisplayName (meta 数据)
#if WITH_EDITOR
		FString DisplayName = ArrayProp->GetMetaData(TEXT("DisplayName"));
		if (!DisplayName.IsEmpty() && DisplayName.Equals(TargetNameStr, ESearchCase::IgnoreCase))
		{
			FoundArrayProp = ArrayProp;
			break;
		}
#endif

		// 方法2: 检查 Property Name (变量名)
		FString PropertyName = ArrayProp->GetName();
		if (PropertyName.Equals(TargetNameStr, ESearchCase::IgnoreCase))
		{
			FoundArrayProp = ArrayProp;
			break;
		}

		// 方法3: 检查 GetDisplayNameText (编辑器显示名称，兼容蓝图结构体)
#if WITH_EDITOR
		FString DisplayNameText = ArrayProp->GetDisplayNameText().ToString();
		if (!DisplayNameText.IsEmpty() && DisplayNameText.Equals(TargetNameStr, ESearchCase::IgnoreCase))
		{
			FoundArrayProp = ArrayProp;
			break;
		}
#endif
	}

	if (!FoundArrayProp)
	{
		// 输出详细的错误信息，列出所有可用的 Array 属性
		FString AvailableProperties;
		for (TFieldIterator<FProperty> PropIt(StructProp->Struct); PropIt; ++PropIt)
		{
			if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(*PropIt))
			{
				if (!AvailableProperties.IsEmpty())
				{
					AvailableProperties += TEXT(", ");
				}

#if WITH_EDITOR
				FString DisplayNameText = ArrayProp->GetDisplayNameText().ToString();
				if (!DisplayNameText.IsEmpty())
				{
					AvailableProperties += DisplayNameText;
				}
				else
#endif
				{
					AvailableProperties += ArrayProp->GetName();
				}
			}
		}

		UE_LOG(LogTemp, Warning,
			TEXT("MapTree_AddArrayItem: 找不到名为 '%s' 的 Array 属性。可用的 Array 属性: %s"),
			*TargetNameStr,
			AvailableProperties.IsEmpty() ? TEXT("(无)") : *AvailableProperties);
	}

	return FoundArrayProp;
}

FSetProperty* UMagnusFuncLib_MapTree::FindSetPropertyByName(const FStructProperty* StructProp, const FName& TargetName)
{
	if (!StructProp || TargetName.IsNone())
	{
		return nullptr;
	}

	const FString TargetNameStr = TargetName.ToString();
	FSetProperty* FoundSetProp = nullptr;

	// 遍历结构体的所有属性
	for (TFieldIterator<FProperty> PropIt(StructProp->Struct); PropIt; ++PropIt)
	{
		FSetProperty* SetProp = CastField<FSetProperty>(*PropIt);
		if (!SetProp)
		{
			continue;
		}

		// 方法1: 检查 DisplayName (meta 数据)
#if WITH_EDITOR
		FString DisplayName = SetProp->GetMetaData(TEXT("DisplayName"));
		if (!DisplayName.IsEmpty() && DisplayName.Equals(TargetNameStr, ESearchCase::IgnoreCase))
		{
			FoundSetProp = SetProp;
			break;
		}
#endif

		// 方法2: 检查 Property Name (变量名)
		FString PropertyName = SetProp->GetName();
		if (PropertyName.Equals(TargetNameStr, ESearchCase::IgnoreCase))
		{
			FoundSetProp = SetProp;
			break;
		}

		// 方法3: 检查 GetDisplayNameText (编辑器显示名称，兼容蓝图结构体)
#if WITH_EDITOR
		FString DisplayNameText = SetProp->GetDisplayNameText().ToString();
		if (!DisplayNameText.IsEmpty() && DisplayNameText.Equals(TargetNameStr, ESearchCase::IgnoreCase))
		{
			FoundSetProp = SetProp;
			break;
		}
#endif
	}

	if (!FoundSetProp)
	{
		// 输出详细的错误信息，列出所有可用的 Set 属性
		FString AvailableProperties;
		for (TFieldIterator<FProperty> PropIt(StructProp->Struct); PropIt; ++PropIt)
		{
			if (FSetProperty* SetProp = CastField<FSetProperty>(*PropIt))
			{
				if (!AvailableProperties.IsEmpty())
				{
					AvailableProperties += TEXT(", ");
				}

#if WITH_EDITOR
				FString DisplayNameText = SetProp->GetDisplayNameText().ToString();
				if (!DisplayNameText.IsEmpty())
				{
					AvailableProperties += DisplayNameText;
				}
				else
#endif
				{
					AvailableProperties += SetProp->GetName();
				}
			}
		}

		UE_LOG(LogTemp, Warning,
			TEXT("MapTree Set 操作: 找不到名为 '%s' 的 Set 属性。可用的 Set 属性: %s"),
			*TargetNameStr,
			AvailableProperties.IsEmpty() ? TEXT("(无)") : *AvailableProperties);
	}

	return FoundSetProp;
}

FMapProperty* UMagnusFuncLib_MapTree::FindMapPropertyByName(const FStructProperty* StructProp, const FName& TargetName)
{
	if (!StructProp || TargetName.IsNone())
	{
		return nullptr;
	}

	const FString TargetNameStr = TargetName.ToString();
	FMapProperty* FoundMapProp = nullptr;

	// 遍历结构体的所有属性
	for (TFieldIterator<FProperty> PropIt(StructProp->Struct); PropIt; ++PropIt)
	{
		FMapProperty* MapProp = CastField<FMapProperty>(*PropIt);
		if (!MapProp)
		{
			continue;
		}

		// 方法1: 检查 DisplayName (meta 数据)
#if WITH_EDITOR
		FString DisplayName = MapProp->GetMetaData(TEXT("DisplayName"));
		if (!DisplayName.IsEmpty() && DisplayName.Equals(TargetNameStr, ESearchCase::IgnoreCase))
		{
			FoundMapProp = MapProp;
			break;
		}
#endif

		// 方法2: 检查 Property Name (变量名)
		FString PropertyName = MapProp->GetName();
		if (PropertyName.Equals(TargetNameStr, ESearchCase::IgnoreCase))
		{
			FoundMapProp = MapProp;
			break;
		}

		// 方法3: 检查 GetDisplayNameText (编辑器显示名称，兼容蓝图结构体)
#if WITH_EDITOR
		FString DisplayNameText = MapProp->GetDisplayNameText().ToString();
		if (!DisplayNameText.IsEmpty() && DisplayNameText.Equals(TargetNameStr, ESearchCase::IgnoreCase))
		{
			FoundMapProp = MapProp;
			break;
		}
#endif
	}

	if (!FoundMapProp)
	{
		// 输出详细的错误信息，列出所有可用的 Map 属性
		FString AvailableProperties;
		for (TFieldIterator<FProperty> PropIt(StructProp->Struct); PropIt; ++PropIt)
		{
			if (FMapProperty* MapProp = CastField<FMapProperty>(*PropIt))
			{
				if (!AvailableProperties.IsEmpty())
				{
					AvailableProperties += TEXT(", ");
				}

#if WITH_EDITOR
				FString DisplayNameText = MapProp->GetDisplayNameText().ToString();
				if (!DisplayNameText.IsEmpty())
				{
					AvailableProperties += DisplayNameText;
				}
				else
#endif
				{
					AvailableProperties += MapProp->GetName();
				}
			}
		}

		UE_LOG(LogTemp, Warning,
			TEXT("MapTree Map 操作: 找不到名为 '%s' 的 Map 属性。可用的 Map 属性: %s"),
			*TargetNameStr,
			AvailableProperties.IsEmpty() ? TEXT("(无)") : *AvailableProperties);
	}

	return FoundMapProp;
}

//================ MapTree																						========

//———————— ArrayItem.Add																							————

DEFINE_FUNCTION(UMagnusFuncLib_MapTree::execMapTree_AddArrayItem)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(NULL);
	void* MapAddr = Stack.MostRecentPropertyAddress;
	FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	// 获取Map的Key属性
	const FProperty* CurrKeyProp = MapProperty->KeyProp;
	const int32 KeyPropertySize = CurrKeyProp->GetElementSize() * CurrKeyProp->ArrayDim;
	void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
	CurrKeyProp->InitializeValue(KeyStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(KeyStorageSpace);

	// 获取 MemberName (FName)
	FName MemberName;
	Stack.StepCompiledIn<FNameProperty>(&MemberName);

	// 获取结构体属性
	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	// 从结构体中查找指定名称的Array属性
	FArrayProperty* ArrayProp = FindArrayPropertyByName(StructProp, MemberName);
	if (!ArrayProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	// 使用Array的元素类型作为Item属性
	const FProperty* CurrItemProp = ArrayProp->Inner;
	const int32 ItemPropertySize = CurrItemProp->GetElementSize() * CurrItemProp->ArrayDim;
	void* ItemStorageSpace = FMemory_Alloca(ItemPropertySize);
	CurrItemProp->InitializeValue(ItemStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(ItemStorageSpace);

	P_FINISH;

	P_NATIVE_BEGIN;
	GenericMapTree_AddArrayItem(MapAddr, MapProperty, KeyStorageSpace, MemberName, ItemStorageSpace);
	P_NATIVE_END;

	CurrItemProp->DestroyValue(ItemStorageSpace);
	CurrKeyProp->DestroyValue(KeyStorageSpace);
}

void UMagnusFuncLib_MapTree::GenericMapTree_AddArrayItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* ItemPtr)
{
	if (!TargetMap || !MapProperty || !KeyPtr || !ItemPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("MapTree_AddArrayItem: 无效的参数"));
		return;
	}

	FScriptMapHelper MapHelper(MapProperty, TargetMap);

	// 获取Map的Value类型（应该是个Struct）
	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("MapTree_AddArrayItem: Map 的 Value 不是结构体类型"));
		return;
	}

	// 查找指定名称的 Array 属性
	FArrayProperty* ArrayProp = FindArrayPropertyByName(StructProp, MemberName);
	if (!ArrayProp)
	{
		// 错误信息已在 FindArrayPropertyByName 中输出
		return;
	}

	// 检查Map中是否已存在此Key
	void* ExistingValuePtr = MapHelper.FindValueFromHash(KeyPtr);

	if (ExistingValuePtr)
	{
		// 情况1：Map中已存在此Key
		// 获取已存在结构体中指定Array成员的指针
		void* ExistingArrayPtr = static_cast<uint8*>(ExistingValuePtr) + ArrayProp->GetOffset_ForInternal();

		// 创建Array助手
		FScriptArrayHelper ArrayHelper(ArrayProp, ExistingArrayPtr);

		// 检查是否已存在相同的值（避免重复）
		bool bItemExists = false;
		for (int32 i = 0; i < ArrayHelper.Num(); ++i)
		{
			if (ArrayProp->Inner->Identical(ArrayHelper.GetRawPtr(i), ItemPtr))
			{
				bItemExists = true;
				break;
			}
		}

		// 如果值不存在，则添加
		if (!bItemExists)
		{
			const int32 NewIndex = ArrayHelper.AddValue();
			ArrayProp->Inner->CopyCompleteValue(ArrayHelper.GetRawPtr(NewIndex), ItemPtr);
		}
	}
	else
	{
		// 情况2：Map中没有此Key
		if (MapHelper.Num() >= MaxSupportedMapSize)
		{
			FFrame::KismetExecutionMessage(
				*FString::Printf(TEXT("MapTree_AddArrayItem: Map '%s' 超出最大容量！"),
					*MapProperty->GetName()),
				ELogVerbosity::Warning,
				UKismetArrayLibrary::ReachedMaximumContainerSizeWarning);
			return;
		}

		// 创建临时的结构体空间
		void* ValueStructure = FMemory::Malloc(StructProp->GetSize(), StructProp->GetMinAlignment());
		StructProp->InitializeValue(ValueStructure);

		// 获取Array在结构体中的偏移
		void* ArrayPtr = static_cast<uint8*>(ValueStructure) + ArrayProp->GetOffset_ForInternal();

		// 初始化Array并添加ItemPtr指向的元素
		FScriptArrayHelper ArrayHelper(ArrayProp, ArrayPtr);
		const int32 NewIndex = ArrayHelper.AddValue();
		ArrayProp->Inner->CopyCompleteValue(ArrayHelper.GetRawPtr(NewIndex), ItemPtr);

		// 将构建好的结构体添加到Map中
		MapHelper.AddPair(KeyPtr, ValueStructure);

		// 清理临时结构体
		StructProp->DestroyValue(ValueStructure);
		FMemory::Free(ValueStructure);
	}
}

//———————— ArrayItem.Remove																							————

DEFINE_FUNCTION(UMagnusFuncLib_MapTree::execMapTree_RemoveArrayItem)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(NULL);
	void* MapAddr = Stack.MostRecentPropertyAddress;
	FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrKeyProp = MapProperty->KeyProp;
	const int32 KeyPropertySize = CurrKeyProp->GetElementSize() * CurrKeyProp->ArrayDim;
	void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
	CurrKeyProp->InitializeValue(KeyStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(KeyStorageSpace);

	FName MemberName;
	Stack.StepCompiledIn<FNameProperty>(&MemberName);

	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	FArrayProperty* ArrayProp = FindArrayPropertyByName(StructProp, MemberName);
	if (!ArrayProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrItemProp = ArrayProp->Inner;
	const int32 ItemPropertySize = CurrItemProp->GetElementSize() * CurrItemProp->ArrayDim;
	void* ItemStorageSpace = FMemory_Alloca(ItemPropertySize);
	CurrItemProp->InitializeValue(ItemStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(ItemStorageSpace);

	P_FINISH;

	P_NATIVE_BEGIN;
	GenericMapTree_RemoveArrayItem(MapAddr, MapProperty, KeyStorageSpace, MemberName, ItemStorageSpace);
	P_NATIVE_END;

	CurrItemProp->DestroyValue(ItemStorageSpace);
	CurrKeyProp->DestroyValue(KeyStorageSpace);
}

void UMagnusFuncLib_MapTree::GenericMapTree_RemoveArrayItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* ItemPtr)
{
	if (!TargetMap || !MapProperty || !KeyPtr || !ItemPtr)
	{
		return;
	}

	FScriptMapHelper MapHelper(MapProperty, TargetMap);
	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		return;
	}

	FArrayProperty* ArrayProp = FindArrayPropertyByName(StructProp, MemberName);
	if (!ArrayProp)
	{
		return;
	}

	void* ExistingValuePtr = MapHelper.FindValueFromHash(KeyPtr);
	if (ExistingValuePtr)
	{
		void* ExistingArrayPtr = static_cast<uint8*>(ExistingValuePtr) + ArrayProp->GetOffset_ForInternal();
		FScriptArrayHelper ArrayHelper(ArrayProp, ExistingArrayPtr);

		for (int32 i = ArrayHelper.Num() - 1; i >= 0; --i)
		{
			if (ArrayProp->Inner->Identical(ArrayHelper.GetRawPtr(i), ItemPtr))
			{
				ArrayHelper.RemoveValues(i, 1);
			}
		}
	}
}

//———————— ArrayItem.Contains																						————

DEFINE_FUNCTION(UMagnusFuncLib_MapTree::execMapTree_ContainsArrayItem)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(NULL);
	void* MapAddr = Stack.MostRecentPropertyAddress;
	FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrKeyProp = MapProperty->KeyProp;
	const int32 KeyPropertySize = CurrKeyProp->GetElementSize() * CurrKeyProp->ArrayDim;
	void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
	CurrKeyProp->InitializeValue(KeyStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(KeyStorageSpace);

	FName MemberName;
	Stack.StepCompiledIn<FNameProperty>(&MemberName);

	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	FArrayProperty* ArrayProp = FindArrayPropertyByName(StructProp, MemberName);
	if (!ArrayProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrItemProp = ArrayProp->Inner;
	const int32 ItemPropertySize = CurrItemProp->GetElementSize() * CurrItemProp->ArrayDim;
	void* ItemStorageSpace = FMemory_Alloca(ItemPropertySize);
	CurrItemProp->InitializeValue(ItemStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(ItemStorageSpace);

	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericMapTree_ContainsArrayItem(MapAddr, MapProperty, KeyStorageSpace, MemberName, ItemStorageSpace);
	P_NATIVE_END;

	CurrItemProp->DestroyValue(ItemStorageSpace);
	CurrKeyProp->DestroyValue(KeyStorageSpace);
}

bool UMagnusFuncLib_MapTree::GenericMapTree_ContainsArrayItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* ItemPtr)
{
	if (!TargetMap || !MapProperty || !KeyPtr || !ItemPtr)
	{
		return false;
	}

	FScriptMapHelper MapHelper(MapProperty, TargetMap);
	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		return false;
	}

	FArrayProperty* ArrayProp = FindArrayPropertyByName(StructProp, MemberName);
	if (!ArrayProp)
	{
		return false;
	}

	void* ExistingValuePtr = MapHelper.FindValueFromHash(KeyPtr);
	if (!ExistingValuePtr)
	{
		return false;
	}

	void* ExistingArrayPtr = static_cast<uint8*>(ExistingValuePtr) + ArrayProp->GetOffset_ForInternal();
	FScriptArrayHelper ArrayHelper(ArrayProp, ExistingArrayPtr);

	for (int32 i = 0; i < ArrayHelper.Num(); ++i)
	{
		if (ArrayProp->Inner->Identical(ArrayHelper.GetRawPtr(i), ItemPtr))
		{
			return true;
		}
	}

	return false;
}

//———————— SetItem.Add																								————

DEFINE_FUNCTION(UMagnusFuncLib_MapTree::execMapTree_AddSetItem)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(NULL);
	void* MapAddr = Stack.MostRecentPropertyAddress;
	FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrKeyProp = MapProperty->KeyProp;
	const int32 KeyPropertySize = CurrKeyProp->GetElementSize() * CurrKeyProp->ArrayDim;
	void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
	CurrKeyProp->InitializeValue(KeyStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(KeyStorageSpace);

	FName MemberName;
	Stack.StepCompiledIn<FNameProperty>(&MemberName);

	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	FSetProperty* SetProp = FindSetPropertyByName(StructProp, MemberName);
	if (!SetProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrItemProp = SetProp->ElementProp;
	const int32 ItemPropertySize = CurrItemProp->GetElementSize() * CurrItemProp->ArrayDim;
	void* ItemStorageSpace = FMemory_Alloca(ItemPropertySize);
	CurrItemProp->InitializeValue(ItemStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(ItemStorageSpace);

	P_FINISH;

	P_NATIVE_BEGIN;
	GenericMapTree_AddSetItem(MapAddr, MapProperty, KeyStorageSpace, MemberName, ItemStorageSpace);
	P_NATIVE_END;

	CurrItemProp->DestroyValue(ItemStorageSpace);
	CurrKeyProp->DestroyValue(KeyStorageSpace);
}

void UMagnusFuncLib_MapTree::GenericMapTree_AddSetItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* ItemPtr)
{
	if (!TargetMap || !MapProperty || !KeyPtr || !ItemPtr)
	{
		return;
	}

	FScriptMapHelper MapHelper(MapProperty, TargetMap);
	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		return;
	}

	FSetProperty* SetProp = FindSetPropertyByName(StructProp, MemberName);
	if (!SetProp)
	{
		return;
	}

	void* ExistingValuePtr = MapHelper.FindValueFromHash(KeyPtr);

	if (ExistingValuePtr)
	{
		void* ExistingSetPtr = static_cast<uint8*>(ExistingValuePtr) + SetProp->GetOffset_ForInternal();
		FScriptSetHelper SetHelper(SetProp, ExistingSetPtr);
		SetHelper.AddElement(ItemPtr);
	}
	else
	{
		if (MapHelper.Num() >= MaxSupportedMapSize)
		{
			FFrame::KismetExecutionMessage(
				*FString::Printf(TEXT("MapTree_AddSetItem: Map '%s' 超出最大容量！"),
					*MapProperty->GetName()),
				ELogVerbosity::Warning,
				UKismetArrayLibrary::ReachedMaximumContainerSizeWarning);
			return;
		}

		void* ValueStructure = FMemory::Malloc(StructProp->GetSize(), StructProp->GetMinAlignment());
		StructProp->InitializeValue(ValueStructure);

		void* SetPtr = static_cast<uint8*>(ValueStructure) + SetProp->GetOffset_ForInternal();
		FScriptSetHelper SetHelper(SetProp, SetPtr);
		SetHelper.AddElement(ItemPtr);

		MapHelper.AddPair(KeyPtr, ValueStructure);

		StructProp->DestroyValue(ValueStructure);
		FMemory::Free(ValueStructure);
	}
}

//———————— SetItem.Remove																							————

DEFINE_FUNCTION(UMagnusFuncLib_MapTree::execMapTree_RemoveSetItem)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(NULL);
	void* MapAddr = Stack.MostRecentPropertyAddress;
	FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrKeyProp = MapProperty->KeyProp;
	const int32 KeyPropertySize = CurrKeyProp->GetElementSize() * CurrKeyProp->ArrayDim;
	void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
	CurrKeyProp->InitializeValue(KeyStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(KeyStorageSpace);

	FName MemberName;
	Stack.StepCompiledIn<FNameProperty>(&MemberName);

	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	FSetProperty* SetProp = FindSetPropertyByName(StructProp, MemberName);
	if (!SetProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrItemProp = SetProp->ElementProp;
	const int32 ItemPropertySize = CurrItemProp->GetElementSize() * CurrItemProp->ArrayDim;
	void* ItemStorageSpace = FMemory_Alloca(ItemPropertySize);
	CurrItemProp->InitializeValue(ItemStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(ItemStorageSpace);

	P_FINISH;

	P_NATIVE_BEGIN;
	GenericMapTree_RemoveSetItem(MapAddr, MapProperty, KeyStorageSpace, MemberName, ItemStorageSpace);
	P_NATIVE_END;

	CurrItemProp->DestroyValue(ItemStorageSpace);
	CurrKeyProp->DestroyValue(KeyStorageSpace);
}

void UMagnusFuncLib_MapTree::GenericMapTree_RemoveSetItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* ItemPtr)
{
	if (!TargetMap || !MapProperty || !KeyPtr || !ItemPtr)
	{
		return;
	}

	FScriptMapHelper MapHelper(MapProperty, TargetMap);
	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		return;
	}

	FSetProperty* SetProp = FindSetPropertyByName(StructProp, MemberName);
	if (!SetProp)
	{
		return;
	}

	void* ExistingValuePtr = MapHelper.FindValueFromHash(KeyPtr);
	if (ExistingValuePtr)
	{
		void* ExistingSetPtr = static_cast<uint8*>(ExistingValuePtr) + SetProp->GetOffset_ForInternal();
		FScriptSetHelper SetHelper(SetProp, ExistingSetPtr);
		SetHelper.RemoveElement(ItemPtr);
	}
}

//———————— SetItem.Contains																							————

DEFINE_FUNCTION(UMagnusFuncLib_MapTree::execMapTree_ContainsSetItem)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(NULL);
	void* MapAddr = Stack.MostRecentPropertyAddress;
	FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrKeyProp = MapProperty->KeyProp;
	const int32 KeyPropertySize = CurrKeyProp->GetElementSize() * CurrKeyProp->ArrayDim;
	void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
	CurrKeyProp->InitializeValue(KeyStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(KeyStorageSpace);

	FName MemberName;
	Stack.StepCompiledIn<FNameProperty>(&MemberName);

	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	FSetProperty* SetProp = FindSetPropertyByName(StructProp, MemberName);
	if (!SetProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrItemProp = SetProp->ElementProp;
	const int32 ItemPropertySize = CurrItemProp->GetElementSize() * CurrItemProp->ArrayDim;
	void* ItemStorageSpace = FMemory_Alloca(ItemPropertySize);
	CurrItemProp->InitializeValue(ItemStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(ItemStorageSpace);

	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericMapTree_ContainsSetItem(MapAddr, MapProperty, KeyStorageSpace, MemberName, ItemStorageSpace);
	P_NATIVE_END;

	CurrItemProp->DestroyValue(ItemStorageSpace);
	CurrKeyProp->DestroyValue(KeyStorageSpace);
}

bool UMagnusFuncLib_MapTree::GenericMapTree_ContainsSetItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* ItemPtr)
{
	if (!TargetMap || !MapProperty || !KeyPtr || !ItemPtr)
	{
		return false;
	}

	FScriptMapHelper MapHelper(MapProperty, TargetMap);
	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		return false;
	}

	FSetProperty* SetProp = FindSetPropertyByName(StructProp, MemberName);
	if (!SetProp)
	{
		return false;
	}

	void* ExistingValuePtr = MapHelper.FindValueFromHash(KeyPtr);
	if (!ExistingValuePtr)
	{
		return false;
	}

	void* ExistingSetPtr = static_cast<uint8*>(ExistingValuePtr) + SetProp->GetOffset_ForInternal();
	FScriptSetHelper SetHelper(SetProp, ExistingSetPtr);

	return SetHelper.FindElementIndex(ItemPtr) != INDEX_NONE;
}

//———————— MapItem.Add																								————

DEFINE_FUNCTION(UMagnusFuncLib_MapTree::execMapTree_AddMapItem)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(NULL);
	void* MapAddr = Stack.MostRecentPropertyAddress;
	FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrKeyProp = MapProperty->KeyProp;
	const int32 KeyPropertySize = CurrKeyProp->GetElementSize() * CurrKeyProp->ArrayDim;
	void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
	CurrKeyProp->InitializeValue(KeyStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(KeyStorageSpace);

	FName MemberName;
	Stack.StepCompiledIn<FNameProperty>(&MemberName);

	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	FMapProperty* InnerMapProp = FindMapPropertyByName(StructProp, MemberName);
	if (!InnerMapProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrSubKeyProp = InnerMapProp->KeyProp;
	const int32 SubKeyPropertySize = CurrSubKeyProp->GetElementSize() * CurrSubKeyProp->ArrayDim;
	void* SubKeyStorageSpace = FMemory_Alloca(SubKeyPropertySize);
	CurrSubKeyProp->InitializeValue(SubKeyStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(SubKeyStorageSpace);

	const FProperty* CurrValueProp = InnerMapProp->ValueProp;
	const int32 ValuePropertySize = CurrValueProp->GetElementSize() * CurrValueProp->ArrayDim;
	void* ValueStorageSpace = FMemory_Alloca(ValuePropertySize);
	CurrValueProp->InitializeValue(ValueStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(ValueStorageSpace);

	P_FINISH;

	P_NATIVE_BEGIN;
	GenericMapTree_AddMapItem(MapAddr, MapProperty, KeyStorageSpace, MemberName, SubKeyStorageSpace, ValueStorageSpace);
	P_NATIVE_END;

	CurrValueProp->DestroyValue(ValueStorageSpace);
	CurrSubKeyProp->DestroyValue(SubKeyStorageSpace);
	CurrKeyProp->DestroyValue(KeyStorageSpace);
}

void UMagnusFuncLib_MapTree::GenericMapTree_AddMapItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* SubKeyPtr, const void* ValuePtr)
{
	if (!TargetMap || !MapProperty || !KeyPtr || !SubKeyPtr || !ValuePtr)
	{
		return;
	}

	FScriptMapHelper MapHelper(MapProperty, TargetMap);
	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		return;
	}

	FMapProperty* InnerMapProp = FindMapPropertyByName(StructProp, MemberName);
	if (!InnerMapProp)
	{
		return;
	}

	void* ExistingValuePtr = MapHelper.FindValueFromHash(KeyPtr);

	if (ExistingValuePtr)
	{
		void* ExistingInnerMapPtr = static_cast<uint8*>(ExistingValuePtr) + InnerMapProp->GetOffset_ForInternal();
		FScriptMapHelper InnerMapHelper(InnerMapProp, ExistingInnerMapPtr);

		if (InnerMapHelper.Num() < MaxSupportedMapSize)
		{
			InnerMapHelper.AddPair(SubKeyPtr, ValuePtr);
		}
	}
	else
	{
		if (MapHelper.Num() >= MaxSupportedMapSize)
		{
			FFrame::KismetExecutionMessage(
				*FString::Printf(TEXT("MapTree_AddMapItem: Map '%s' 超出最大容量！"),
					*MapProperty->GetName()),
				ELogVerbosity::Warning,
				UKismetArrayLibrary::ReachedMaximumContainerSizeWarning);
			return;
		}

		void* ValueStructure = FMemory::Malloc(StructProp->GetSize(), StructProp->GetMinAlignment());
		StructProp->InitializeValue(ValueStructure);

		void* InnerMapPtr = static_cast<uint8*>(ValueStructure) + InnerMapProp->GetOffset_ForInternal();
		FScriptMapHelper InnerMapHelper(InnerMapProp, InnerMapPtr);
		InnerMapHelper.AddPair(SubKeyPtr, ValuePtr);

		MapHelper.AddPair(KeyPtr, ValueStructure);

		StructProp->DestroyValue(ValueStructure);
		FMemory::Free(ValueStructure);
	}
}

//———————— MapItem.Remove																							————

DEFINE_FUNCTION(UMagnusFuncLib_MapTree::execMapTree_RemoveMapItem)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(NULL);
	void* MapAddr = Stack.MostRecentPropertyAddress;
	FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrKeyProp = MapProperty->KeyProp;
	const int32 KeyPropertySize = CurrKeyProp->GetElementSize() * CurrKeyProp->ArrayDim;
	void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
	CurrKeyProp->InitializeValue(KeyStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(KeyStorageSpace);

	FName MemberName;
	Stack.StepCompiledIn<FNameProperty>(&MemberName);

	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	FMapProperty* InnerMapProp = FindMapPropertyByName(StructProp, MemberName);
	if (!InnerMapProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrSubKeyProp = InnerMapProp->KeyProp;
	const int32 SubKeyPropertySize = CurrSubKeyProp->GetElementSize() * CurrSubKeyProp->ArrayDim;
	void* SubKeyStorageSpace = FMemory_Alloca(SubKeyPropertySize);
	CurrSubKeyProp->InitializeValue(SubKeyStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(SubKeyStorageSpace);

	P_FINISH;

	P_NATIVE_BEGIN;
	GenericMapTree_RemoveMapItem(MapAddr, MapProperty, KeyStorageSpace, MemberName, SubKeyStorageSpace);
	P_NATIVE_END;

	CurrSubKeyProp->DestroyValue(SubKeyStorageSpace);
	CurrKeyProp->DestroyValue(KeyStorageSpace);
}

void UMagnusFuncLib_MapTree::GenericMapTree_RemoveMapItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* SubKeyPtr)
{
	if (!TargetMap || !MapProperty || !KeyPtr || !SubKeyPtr)
	{
		return;
	}

	FScriptMapHelper MapHelper(MapProperty, TargetMap);
	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		return;
	}

	FMapProperty* InnerMapProp = FindMapPropertyByName(StructProp, MemberName);
	if (!InnerMapProp)
	{
		return;
	}

	void* ExistingValuePtr = MapHelper.FindValueFromHash(KeyPtr);
	if (ExistingValuePtr)
	{
		void* ExistingInnerMapPtr = static_cast<uint8*>(ExistingValuePtr) + InnerMapProp->GetOffset_ForInternal();
		FScriptMapHelper InnerMapHelper(InnerMapProp, ExistingInnerMapPtr);
		InnerMapHelper.RemovePair(SubKeyPtr);
	}
}

//———————— MapItem.Contains																							————

DEFINE_FUNCTION(UMagnusFuncLib_MapTree::execMapTree_ContainsMapItem)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FMapProperty>(NULL);
	void* MapAddr = Stack.MostRecentPropertyAddress;
	FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
	if (!MapProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrKeyProp = MapProperty->KeyProp;
	const int32 KeyPropertySize = CurrKeyProp->GetElementSize() * CurrKeyProp->ArrayDim;
	void* KeyStorageSpace = FMemory_Alloca(KeyPropertySize);
	CurrKeyProp->InitializeValue(KeyStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(KeyStorageSpace);

	FName MemberName;
	Stack.StepCompiledIn<FNameProperty>(&MemberName);

	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	FMapProperty* InnerMapProp = FindMapPropertyByName(StructProp, MemberName);
	if (!InnerMapProp)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	const FProperty* CurrSubKeyProp = InnerMapProp->KeyProp;
	const int32 SubKeyPropertySize = CurrSubKeyProp->GetElementSize() * CurrSubKeyProp->ArrayDim;
	void* SubKeyStorageSpace = FMemory_Alloca(SubKeyPropertySize);
	CurrSubKeyProp->InitializeValue(SubKeyStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(SubKeyStorageSpace);

	const FProperty* CurrValueProp = InnerMapProp->ValueProp;
	const int32 ValuePropertySize = CurrValueProp->GetElementSize() * CurrValueProp->ArrayDim;
	void* ValueStorageSpace = FMemory_Alloca(ValuePropertySize);
	CurrValueProp->InitializeValue(ValueStorageSpace);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(ValueStorageSpace);

	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericMapTree_ContainsMapItem(MapAddr, MapProperty, KeyStorageSpace, MemberName, SubKeyStorageSpace, ValueStorageSpace);
	P_NATIVE_END;

	CurrValueProp->DestroyValue(ValueStorageSpace);
	CurrSubKeyProp->DestroyValue(SubKeyStorageSpace);
	CurrKeyProp->DestroyValue(KeyStorageSpace);
}

bool UMagnusFuncLib_MapTree::GenericMapTree_ContainsMapItem(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, const FName& MemberName, const void* SubKeyPtr, const void* ValuePtr)
{
	if (!TargetMap || !MapProperty || !KeyPtr || !SubKeyPtr || !ValuePtr)
	{
		return false;
	}

	FScriptMapHelper MapHelper(MapProperty, TargetMap);
	const FStructProperty* StructProp = CastField<FStructProperty>(MapProperty->ValueProp);
	if (!StructProp)
	{
		return false;
	}

	FMapProperty* InnerMapProp = FindMapPropertyByName(StructProp, MemberName);
	if (!InnerMapProp)
	{
		return false;
	}

	void* ExistingValuePtr = MapHelper.FindValueFromHash(KeyPtr);
	if (!ExistingValuePtr)
	{
		return false;
	}

	void* ExistingInnerMapPtr = static_cast<uint8*>(ExistingValuePtr) + InnerMapProp->GetOffset_ForInternal();
	FScriptMapHelper InnerMapHelper(InnerMapProp, ExistingInnerMapPtr);

	void* ExistingInnerValuePtr = InnerMapHelper.FindValueFromHash(SubKeyPtr);
	if (!ExistingInnerValuePtr)
	{
		return false;
	}

	return InnerMapProp->ValueProp->Identical(ExistingInnerValuePtr, ValuePtr);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
