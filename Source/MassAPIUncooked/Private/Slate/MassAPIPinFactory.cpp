// Leroy Works & Ember, All Rights Reserved.

#include "Slate/MassAPIPinFactory.h"

#if WITH_EDITOR
#include "Slate/SGraphPinMassAPI.h"
#include "OperationFragment/K2Node_SetMassFragment.h"
#include "OperationFragment/K2Node_GetMassFragment.h"
#include "OperationFragment/K2Node_RemoveMassFragment.h"
#include "OperationFragment/K2Node_HasMassFragment.h"
#include "MassAPIUncooked/Public/OperationTag/K2Node_AddMassTag.h"
#include "MassAPIUncooked/Public/OperationTag/K2Node_RemoveMassTag.h"
#include "MassAPIUncooked/Public/OperationTag/K2Node_HasMassTag.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#endif

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if WITH_EDITOR

//================ Factory.Override																				========

TSharedPtr<SGraphPin> FMassAPIPinFactory::CreatePin(UEdGraphPin* InPin) const
{
	// 检查引脚类型是否为 Struct（结构体）
	if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
	{
		// 检查引脚的子类别是否为 Self（即引脚类型本身是一个类型选择器）
		if (InPin->PinType.PinSubCategory == UEdGraphSchema_K2::PSC_Self)
		{
			// 检查引脚的拥有者节点
			UObject* Outer = InPin->GetOuter();

			// 如果是 UK2Node_SetMassFragment 节点的 FragmentType 引脚
			if (Outer && Outer->IsA(UK2Node_SetMassFragment::StaticClass()))
			{
				// 检查引脚名称是否为 FragmentType
				if (InPin->PinName == TEXT("FragmentType"))
				{
					// 创建并返回自定义的 Fragment 选择器 Pin
					return SNew(SGraphPinMassFragmentType, InPin);
				}
			}

			// 如果是 UK2Node_GetMassFragment 节点的 FragmentType 引脚
			if (Outer && Outer->IsA(UK2Node_GetMassFragment::StaticClass()))
			{
				// 检查引脚名称是否为 FragmentType
				if (InPin->PinName == TEXT("FragmentType"))
				{
					// 创建并返回自定义的 Fragment 选择器 Pin
					return SNew(SGraphPinMassFragmentType, InPin);
				}
			}

			// 如果是 UK2Node_RemoveMassFragment 节点的 FragmentType 引脚
			if (Outer && Outer->IsA(UK2Node_RemoveMassFragment::StaticClass()))
			{
				// 检查引脚名称是否为 FragmentType
				if (InPin->PinName == TEXT("FragmentType"))
				{
					// 创建并返回自定义的 Fragment 选择器 Pin
					return SNew(SGraphPinMassFragmentType, InPin);
				}
			}

			// 如果是 UK2Node_HasMassFragment 节点的 FragmentType 引脚
			if (Outer && Outer->IsA(UK2Node_HasMassFragment::StaticClass()))
			{
				// 检查引脚名称是否为 FragmentType
				if (InPin->PinName == TEXT("FragmentType"))
				{
					// 创建并返回自定义的 Fragment 选择器 Pin
					return SNew(SGraphPinMassFragmentType, InPin);
				}
			}

			// 如果是 UK2Node_AddMassTag 节点的 TagType 引脚
			if (Outer && Outer->IsA(UK2Node_AddMassTag::StaticClass()))
			{
				// 检查引脚名称是否为 TagType
				if (InPin->PinName == TEXT("TagType"))
				{
					// 创建并返回自定义的 Tag 选择器 Pin
					return SNew(SGraphPinMassTagType, InPin);
				}
			}

			// 如果是 UK2Node_RemoveMassTag 节点的 TagType 引脚
			if (Outer && Outer->IsA(UK2Node_RemoveMassTag::StaticClass()))
			{
				// 检查引脚名称是否为 TagType
				if (InPin->PinName == TEXT("TagType"))
				{
					// 创建并返回自定义的 Tag 选择器 Pin
					return SNew(SGraphPinMassTagType, InPin);
				}
			}

			// 如果是 UK2Node_HasMassTag 节点的 TagType 引脚
			if (Outer && Outer->IsA(UK2Node_HasMassTag::StaticClass()))
			{
				// 检查引脚名称是否为 TagType
				if (InPin->PinName == TEXT("TagType"))
				{
					// 创建并返回自定义的 Tag 选择器 Pin
					return SNew(SGraphPinMassTagType, InPin);
				}
			}
		}
	}

	// 不符合条件，返回 nullptr 让其他工厂处理
	return nullptr;
}

#endif

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
