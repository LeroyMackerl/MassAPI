#include "Property/MagnusNodesStruct.h"
#include "UObject/TextProperty.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if WITH_EDITOR

//================ FStructMemberPropertyFilterRegistry															========

TMap<FName, FPropertyFilterFunc>& FStructMemberPropertyFilterRegistry::GetFilterMap()
{
	// 静态局部变量，线程安全且懒加载
	static TMap<FName, FPropertyFilterFunc> FilterMap;
	return FilterMap;
}

void FStructMemberPropertyFilterRegistry::RegisterFilter(FName FilterName, FPropertyFilterFunc FilterFunc)
{
	if (!FilterName.IsNone() && FilterFunc)
	{
		GetFilterMap().Add(FilterName, MoveTemp(FilterFunc));
	}
}

FPropertyFilterFunc* FStructMemberPropertyFilterRegistry::GetFilter(FName FilterName)
{
	return GetFilterMap().Find(FilterName);
}

void FStructMemberPropertyFilterRegistry::UnregisterFilter(FName FilterName)
{
	GetFilterMap().Remove(FilterName);
}

void FStructMemberPropertyFilterRegistry::ClearAllFilters()
{
	GetFilterMap().Empty();
}

#endif // WITH_EDITOR

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//================ FStructMemberTreeNode																		========

FText FStructMemberTreeNode::GetPropertyTypeText() const
{
	if (!Property)
	{
		return FText::FromString(TEXT("Unknown"));
	}

	return FText::FromString(Property->GetCPPType());
}

bool FStructMemberTreeNode::IsStructType() const
{
	return Children.Num() > 0;
}

void* FStructMemberTreeNode::GetMemberDataPtr(void* StructData) const
{
	if (!StructData || !Property)
	{
		return nullptr;
	}

	// 使用偏移路径计算最终的数据指针
	uint8* DataPtr = static_cast<uint8*>(StructData);

	for (int32 Offset : PropertyOffsets)
	{
		DataPtr += Offset;
	}

	return DataPtr;
}

const void* FStructMemberTreeNode::GetMemberDataPtr(const void* StructData) const
{
	if (!StructData || !Property)
	{
		return nullptr;
	}

	// 使用偏移路径计算最终的数据指针
	const uint8* DataPtr = static_cast<const uint8*>(StructData);

	for (int32 Offset : PropertyOffsets)
	{
		DataPtr += Offset;
	}

	return DataPtr;
}

FString FStructMemberTreeNode::GetMemberValueAsString(const void* StructData) const
{
	if (!StructData || !Property)
	{
		return TEXT("Invalid");
	}

	const void* MemberDataPtr = GetMemberDataPtr(StructData);
	if (!MemberDataPtr)
	{
		return TEXT("Invalid Ptr");
	}

	// 将属性值导出为字符串
	FString ValueString;
	Property->ExportTextItem_Direct(ValueString, MemberDataPtr, nullptr, nullptr, PPF_None);

	return ValueString;
}

FString FStructMemberTreeNode::GetGraphPinClassName() const
{
	if (!Property)
	{
		return TEXT("SGraphPin");
	}

	// 根据属性类型返回对应的 GraphPin 类名
	if (Property->IsA<FBoolProperty>())
	{
		return TEXT("SGraphPinBool");
	}
	else if (Property->IsA<FIntProperty>())
	{
		return TEXT("SGraphPinInteger");
	}
	else if (Property->IsA<FInt64Property>())
	{
		return TEXT("SGraphPinInteger");
	}
	else if (Property->IsA<FFloatProperty>())
	{
		return TEXT("SGraphPinNum");
	}
	else if (Property->IsA<FDoubleProperty>())
	{
		return TEXT("SGraphPinNum");
	}
	else if (Property->IsA<FNameProperty>())
	{
		return TEXT("SGraphPinName");
	}
	else if (Property->IsA<FStrProperty>())
	{
		return TEXT("SGraphPinString");
	}
	else if (Property->IsA<FTextProperty>())
	{
		return TEXT("SGraphPinText");
	}
	else if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		if (StructProperty->Struct)
		{
			FName StructName = StructProperty->Struct->GetFName();

			// 常见的结构体类型
			if (StructName == NAME_Vector)
			{
				return TEXT("SGraphPinVector");
			}
			else if (StructName == NAME_Vector2D)
			{
				return TEXT("SGraphPinVector2D");
			}
			else if (StructName == NAME_Vector4)
			{
				return TEXT("SGraphPinVector4");
			}
			else if (StructName == NAME_Rotator)
			{
				return TEXT("SGraphPinRotator");
			}
			else if (StructName == NAME_Transform)
			{
				return TEXT("SGraphPinTransform");
			}
			else if (StructName == NAME_Color || StructName == NAME_LinearColor)
			{
				return TEXT("SGraphPinColor");
			}
			else
			{
				return TEXT("SGraphPinStruct");
			}
		}
	}
	else if (Property->IsA<FObjectProperty>())
	{
		return TEXT("SGraphPinObject");
	}
	else if (Property->IsA<FClassProperty>())
	{
		return TEXT("SGraphPinClass");
	}
	else if (Property->IsA<FArrayProperty>())
	{
		return TEXT("SGraphPinArray");
	}

	// 默认
	return TEXT("SGraphPin");
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
