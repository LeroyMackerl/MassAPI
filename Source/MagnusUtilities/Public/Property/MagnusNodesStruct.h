#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "CustomProperty/Magnus_CustomPropertyHelper.h"
#endif

#include "MagnusNodesStruct.generated.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if WITH_EDITOR
// 属性过滤器函数签名
using FPropertyFilterFunc = TFunction<bool(const FProperty*)>;

// 属性过滤器注册表
class MAGNUSUTILITIES_API FStructMemberPropertyFilterRegistry
{
public:
	// 注册一个命名过滤器
	static void RegisterFilter(FName FilterName, FPropertyFilterFunc FilterFunc);

	// 获取已注册的过滤器（如果不存在返回nullptr）
	static FPropertyFilterFunc* GetFilter(FName FilterName);

	// 取消注册过滤器
	static void UnregisterFilter(FName FilterName);

	// 清空所有过滤器
	static void ClearAllFilters();

private:
	// 获取过滤器注册表单例
	static TMap<FName, FPropertyFilterFunc>& GetFilterMap();
};
#endif

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//结构体成员树节点
USTRUCT(BlueprintType)
struct MAGNUSUTILITIES_API FStructMemberTreeNode
{
	GENERATED_BODY()

	// 成员名称
	UPROPERTY(BlueprintReadOnly, Category = "MassAPI|StructMember")
	FString MemberName;

	// 成员显示名称（用于 UI 显示）
	UPROPERTY(BlueprintReadOnly, Category = "MassAPI|StructMember")
	FText DisplayName;

	// 属性路径（从根结构体到当前成员的完整路径）
	// 例如：["TransformData", "Location", "X"]
	UPROPERTY(BlueprintReadOnly, Category = "MassAPI|StructMember")
	TArray<FString> PropertyPath;

	// 属性偏移路径（每一层的偏移量）
	UPROPERTY(BlueprintReadOnly, Category = "MassAPI|StructMember")
	TArray<int32> PropertyOffsets;

	// 嵌套深度（0 为根级成员）
	UPROPERTY(BlueprintReadOnly, Category = "MassAPI|StructMember")
	int32 Depth = 0;

	// 默认构造函数
	FStructMemberTreeNode() = default;

	// 获取完整的属性路径字符串（用点号分隔）
	// 例如："TransformData.Location.X"
	FString GetPropertyPathString() const
	{
		return FString::Join(PropertyPath, TEXT("."));
	}

	// 获取属性类型的显示文本（需要在运行时通过反射获取）
	FText GetPropertyTypeText() const;

	// 检查是否是结构体类型（有子成员）
	bool IsStructType() const;

	//================ Utility.Functions																		========

	// 从结构体实例中获取此成员的数据指针
	// @param StructData - 结构体实例的指针
	// @return 成员数据的指针，如果路径无效则返回 nullptr
	void* GetMemberDataPtr(void* StructData) const;

	// 从结构体实例中获取此成员的数据指针（const 版本）
	const void* GetMemberDataPtr(const void* StructData) const;

	// 获取此成员的值（以字符串形式）
	// @param StructData - 结构体实例的指针
	// @return 成员值的字符串表示
	FString GetMemberValueAsString(const void* StructData) const;

	// 根据此成员的类型信息创建对应的 SGraphPin 类名
	// @return GraphPin 的类名（例如 "SGraphPinVector", "SGraphPinFloat" 等）
	FString GetGraphPinClassName() const;

	// 以下字段仅在编辑器中使用，不序列化
	// 属性指针（运行时无效）
	FProperty* Property = nullptr;

	// 所属的结构体类型（对于嵌套结构体成员）
	// 使用弱指针避免垃圾回收问题
	TWeakObjectPtr<const UScriptStruct> OwnerStruct = nullptr;

	// 子节点（如果成员是结构体，则包含其成员）
	TArray<TSharedPtr<FStructMemberTreeNode>> Children;

	// 父节点
	TWeakPtr<FStructMemberTreeNode> Parent;
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// 结构体成员引用
USTRUCT(BlueprintType)
struct MAGNUSUTILITIES_API FStructMemberReference
{
	GENERATED_BODY()

	/** 目标结构体类型 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|StructMember")
	TObjectPtr<UScriptStruct> StructType = nullptr;

	/** 成员路径（例如: "Transform"） */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|StructMember")
	FString MemberPath;

	// 默认构造函数
	FStructMemberReference() = default;

	// 构造函数
	FStructMemberReference(UScriptStruct* InStructType, const FString& InMemberPath)
		: StructType(InStructType)
		, MemberPath(InMemberPath)
	{
	}

	/** 检查引用是否有效 */
	bool IsValid() const
	{
		return StructType != nullptr && !MemberPath.IsEmpty();
	}

	/** 清空引用 */
	void Reset()
	{
		StructType = nullptr;
		MemberPath.Empty();
	}

	/** 获取完整的路径描述 */
	FString GetFullDescription() const
	{
		if (!IsValid())
		{
			return TEXT("Invalid");
		}
		return FString::Printf(TEXT("%s::%s"), *StructType->GetName(), *MemberPath);
	}

	// 比较运算符
	bool operator==(const FStructMemberReference& Other) const
	{
		return StructType == Other.StructType && MemberPath == Other.MemberPath;
	}

	bool operator!=(const FStructMemberReference& Other) const
	{
		return !(*this == Other);
	}
};

#if WITH_EDITOR
BEGIN_MCPH_DEFINE(FStructMemberReference)

	MCPH_HEADER_CUSTOM
	MCPH_CHILDREN_CUSTOM

	// 属性句柄
	TSharedPtr<IPropertyHandle> MainPropertyHandle;
	TSharedPtr<IPropertyHandle> StructTypeHandle;
	TSharedPtr<IPropertyHandle> MemberPathHandle;

	// 当前选中的结构体类型
	TWeakObjectPtr<const UScriptStruct> CurrentStructType = nullptr;

	// 当前使用的过滤器函数
	FPropertyFilterFunc CurrentFilterFunc = nullptr;

	// 回调函数
	void OnStructTypeChanged();

	// 获取当前显示的文本
	FText GetCurrentValueText() const;

	// 构建第一层成员列表
	void BuildFirstLevelMemberRows(IDetailChildrenBuilder& ChildBuilder);

	// 检查路径是否被选中
	bool IsPathSelected(const FString& Path) const;

	// 设置选中的路径（单选）
	void SetSelectedPath(const FString& Path);

	// ====== 过滤相关 ======

	// 从 Meta 加载过滤器
	void LoadFilterFromMeta();

	// 检查属性是否应该显示（根据过滤器）
	bool ShouldShowProperty(const FProperty* Property) const;

	// ====== 内置过滤器 ======

	// 判断是否是容器类型
	static bool IsContainerProperty(const FProperty* Property);

	// 判断是否是单一类型
	static bool IsSingleProperty(const FProperty* Property);

	// 判断是否是数值类型
	static bool IsNumericProperty(const FProperty* Property);

END_MCPH_DEFINE
#endif

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// 递归结构体成员引用
USTRUCT(BlueprintType)
struct MAGNUSUTILITIES_API FStructRecursiveMemberReference
{
	GENERATED_BODY()

	/** 目标结构体类型 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|StructMember")
	TObjectPtr<UScriptStruct> StructType = nullptr;

	/** 成员路径列表（支持多选，例如: ["Transform.Location.X", "Transform.Rotation.Y"]） */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MassAPI|StructMember")
	TArray<FString> MemberPaths;

	// 默认构造函数
	FStructRecursiveMemberReference() = default;

	// 构造函数
	FStructRecursiveMemberReference(UScriptStruct* InStructType, const TArray<FString>& InMemberPaths)
		: StructType(InStructType)
		, MemberPaths(InMemberPaths)
	{
	}

	/** 检查引用是否有效 */
	bool IsValid() const
	{
		return StructType != nullptr && MemberPaths.Num() > 0;
	}

	/** 清空引用 */
	void Reset()
	{
		StructType = nullptr;
		MemberPaths.Empty();
	}

	/** 获取完整的路径描述 */
	FString GetFullDescription() const
	{
		if (!StructType || MemberPaths.Num() == 0)
		{
			return TEXT("Invalid");
		}

		if (MemberPaths.Num() == 1)
		{
			return FString::Printf(TEXT("%s::%s"), *StructType->GetName(), *MemberPaths[0]);
		}

		return FString::Printf(TEXT("%s::[%d members]"), *StructType->GetName(), MemberPaths.Num());
	}

	// 比较运算符
	bool operator==(const FStructRecursiveMemberReference& Other) const
	{
		if (StructType != Other.StructType || MemberPaths.Num() != Other.MemberPaths.Num())
			return false;

		for (const FString& Path : MemberPaths)
		{
			if (!Other.MemberPaths.Contains(Path))
				return false;
		}
		return true;
	}

	bool operator!=(const FStructRecursiveMemberReference& Other) const
	{
		return !(*this == Other);
	}
};

#if WITH_EDITOR
BEGIN_MCPH_DEFINE(FStructRecursiveMemberReference)

	MCPH_HEADER_CUSTOM
	MCPH_CHILDREN_CUSTOM

	// 属性句柄
	TSharedPtr<IPropertyHandle> StructTypeHandle;
	TSharedPtr<IPropertyHandle> MemberPathsHandle;

	// 当前选中的结构体类型
	// 使用弱指针避免垃圾回收问题
	TWeakObjectPtr<const UScriptStruct> CurrentStructType = nullptr;

	// 回调函数
	void OnStructTypeChanged();

	// 获取当前显示的文本
	FText GetCurrentValueText() const;

	// 清空选择
	FReply OnClearClicked();

	// 递归构建成员行（在ChildBuilder中）
	void BuildRecursiveMemberRows(
		IDetailChildrenBuilder& ChildBuilder,
		const UScriptStruct* StructType,
		const TArray<FString>& ParentPath);

	// 递归构建成员行（在Group中）
	void BuildRecursiveMemberRowsForGroup(
		IDetailGroup& Group,
		const UScriptStruct* StructType,
		const TArray<FString>& ParentPath);

	// 辅助函数：检查路径是否被选中
	bool IsPathSelected(const FString& Path) const;

	// 辅助函数：检查路径A是否是路径B的父节点
	// 例如：IsParentOf("Transform.Location", "Transform.Location.X") 返回 true
	bool IsParentOf(const FString& ParentPath, const FString& ChildPath) const;

	// 辅助函数：检查路径A是否是路径B的子节点
	bool IsChildOf(const FString& ChildPath, const FString& ParentPath) const;

	// 辅助函数：切换路径的选中状态（处理层级互斥）
	void TogglePathSelection(const FString& Path);

	// 辅助函数：移除所有与指定路径有父子关系的路径
	void RemoveConflictingPaths(const FString& Path, TArray<FString>& Paths) const;

END_MCPH_DEFINE
#endif

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

// 递归结构体成员操作辅助函数
namespace StructMemberHelper
{
	// 在结构体中查找指定名称的属性
	static FProperty* FindPropertyInStruct(const UStruct* InStruct, const FName InName)
	{
		if (!InStruct || InName.IsNone())
		{
			return nullptr;
		}

		return FindFProperty<FProperty>(InStruct, InName);
	}

	// 根据单级成员名称查找属性（仅第一级，如 "Translation"，不支持嵌套路径）
	static FProperty* FindPropertyBySinglePath(const UScriptStruct* InStruct, const FString& MemberPath)
	{
		if (!InStruct || MemberPath.IsEmpty())
		{
			return nullptr;
		}

		// 只支持第一级成员，不支持嵌套路径（如 "Translation.X"）
		if (MemberPath.Contains(TEXT(".")))
		{
			return nullptr;
		}

		// 直接查找第一级属性
		return FindFProperty<FProperty>(InStruct, FName(*MemberPath));
	}

	// 根据递归成员路径查找属性（支持嵌套，如 "Translation.X"）
	static FProperty* FindPropertyByRecursivePath(const UScriptStruct* InStruct, const FString& MemberPath)
	{
		if (!InStruct || MemberPath.IsEmpty())
		{
			return nullptr;
		}

		// 分割路径
		TArray<FString> PathParts;
		MemberPath.ParseIntoArray(PathParts, TEXT("."), true);

		if (PathParts.Num() == 0)
		{
			return nullptr;
		}

		// 遍历路径，逐级查找
		const UStruct* CurrentStruct = InStruct;
		FProperty* CurrentProperty = nullptr;

		for (int32 i = 0; i < PathParts.Num(); ++i)
		{
			const FString& PartName = PathParts[i];
			CurrentProperty = FindFProperty<FProperty>(CurrentStruct, FName(*PartName));

			if (!CurrentProperty)
			{
				return nullptr;
			}

			// 如果不是最后一级，检查是否是结构体属性
			if (i < PathParts.Num() - 1)
			{
				FStructProperty* StructProperty = CastField<FStructProperty>(CurrentProperty);
				if (!StructProperty || !StructProperty->Struct)
				{
					return nullptr;
				}
				CurrentStruct = StructProperty->Struct;
			}
		}

		return CurrentProperty;
	}

#if WITH_EDITOR
	// 检查是否可以为属性创建引脚
	static bool CanCreatePinForProperty(const FProperty* Property)
	{
		if (!Property)
		{
			return false;
		}

		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		FEdGraphPinType DumbGraphPinType;
		return Schema->ConvertPropertyToPinType(Property, /*out*/ DumbGraphPinType);
	}
#endif

	// 计算成员路径的偏移索引序列（用于排序）
	// 例如：Transform.Location.X -> [0, 1, 0]（假设Transform是第0个，Location是Transform中第1个，X是Location中第0个）
	static TArray<int32> GetMemberOffsetIndices(const UScriptStruct* InStruct, const FString& MemberPath)
	{
		TArray<int32> OffsetIndices;

		if (!InStruct || MemberPath.IsEmpty())
		{
			return OffsetIndices;
		}

		// 分割路径
		TArray<FString> PathParts;
		MemberPath.ParseIntoArray(PathParts, TEXT("."), true);

		if (PathParts.Num() == 0)
		{
			return OffsetIndices;
		}

		// 遍历路径，逐级查找索引
		const UStruct* CurrentStruct = InStruct;

		for (int32 i = 0; i < PathParts.Num(); ++i)
		{
			const FString& PartName = PathParts[i];

			// 在当前结构体中查找该成员的索引
			int32 MemberIndex = 0;
			bool bFound = false;

			for (TFieldIterator<FProperty> It(CurrentStruct); It; ++It)
			{
				FProperty* Property = *It;
				if (Property && Property->GetName() == PartName)
				{
					OffsetIndices.Add(MemberIndex);
					bFound = true;

					// 如果不是最后一级，检查是否是结构体属性
					if (i < PathParts.Num() - 1)
					{
						FStructProperty* StructProperty = CastField<FStructProperty>(Property);
						if (StructProperty && StructProperty->Struct)
						{
							CurrentStruct = StructProperty->Struct;
						}
						else
						{
							// 路径无效
							return TArray<int32>();
						}
					}
					break;
				}
				MemberIndex++;
			}

			if (!bFound)
			{
				// 成员未找到
				return TArray<int32>();
			}
		}

		return OffsetIndices;
	}

	// 比较两个偏移索引序列（深度优先）
	// 返回 true 表示 A 应该排在 B 前面
	static bool CompareOffsetIndices(const TArray<int32>& A, const TArray<int32>& B)
	{
		int32 MinLen = FMath::Min(A.Num(), B.Num());

		for (int32 i = 0; i < MinLen; ++i)
		{
			if (A[i] < B[i])
			{
				return true;
			}
			else if (A[i] > B[i])
			{
				return false;
			}
			// 相等则继续比较下一级
		}

		// 如果前面都相等，短的排在前面
		return A.Num() < B.Num();
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
