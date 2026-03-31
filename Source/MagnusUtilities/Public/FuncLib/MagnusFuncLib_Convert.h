/*
* Magnus Utilities
* Author: Ember, All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Script.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UnrealType.h"
#include "UObject/ScriptMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "MagnusFuncLib_Convert.generated.h"

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

UCLASS()
class MAGNUSUTILITIES_API UMagnusFuncLib_Convert : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	//================ Scalar Precision Conversion ========

	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "To Float (Double)", CompactNodeTitle = "->", BlueprintAutocast))
	static float VariableConvert_DoubleToFloat(double Value);

	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "To Double (Float)", CompactNodeTitle = "->", BlueprintAutocast))
	static double VariableConvert_FloatToDouble(float Value);

	//================ Scalar Identity (for literal value handling) ========

	/** Identity function for double - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Double)", BlueprintInternalUseOnly = "true"))
	static double Conv_IdentityDouble(double Value);

	/** Identity function for float - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Float)", BlueprintInternalUseOnly = "true"))
	static float Conv_IdentityFloat(float Value);

	/** Identity function for bool - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Bool)", BlueprintInternalUseOnly = "true"))
	static bool Conv_IdentityBool(bool Value);

	/** Identity function for int32 - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Int)", BlueprintInternalUseOnly = "true"))
	static int32 Conv_IdentityInt(int32 Value);

	/** Identity function for int64 - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Int64)", BlueprintInternalUseOnly = "true"))
	static int64 Conv_IdentityInt64(int64 Value);

	/** Identity function for uint8 (byte) - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Byte)", BlueprintInternalUseOnly = "true"))
	static uint8 Conv_IdentityByte(uint8 Value);

	/** Identity function for FName - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Name)", BlueprintInternalUseOnly = "true"))
	static FName Conv_IdentityName(FName Value);

	/** Identity function for FString - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (String)", BlueprintInternalUseOnly = "true"))
	static FString Conv_IdentityString(const FString& Value);

	/** Identity function for FText - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Text)", BlueprintInternalUseOnly = "true"))
	static FText Conv_IdentityText(const FText& Value);

	//================ Struct Identity (for literal value handling) ========

	/** Identity function for FVector2D - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Vector2D)", BlueprintInternalUseOnly = "true"))
	static FVector2D Conv_IdentityVector2D(FVector2D Value);

	/** Identity function for FVector - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Vector)", BlueprintInternalUseOnly = "true"))
	static FVector Conv_IdentityVector(FVector Value);

	/** Identity function for FVector4 - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Vector4)", BlueprintInternalUseOnly = "true"))
	static FVector4 Conv_IdentityVector4(FVector4 Value);

	/** Identity function for FRotator - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Rotator)", BlueprintInternalUseOnly = "true"))
	static FRotator Conv_IdentityRotator(FRotator Value);

	/** Identity function for FQuat - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Quat)", BlueprintInternalUseOnly = "true"))
	static FQuat Conv_IdentityQuat(FQuat Value);

	/** Identity function for FTransform - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Transform)", BlueprintInternalUseOnly = "true"))
	static FTransform Conv_IdentityTransform(FTransform Value);

	/** Identity function for FMatrix - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Matrix)", BlueprintInternalUseOnly = "true"))
	static FMatrix Conv_IdentityMatrix(FMatrix Value);

	/** Identity function for FPlane - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Plane)", BlueprintInternalUseOnly = "true"))
	static FPlane Conv_IdentityPlane(FPlane Value);

	/** Identity function for FBox - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Box)", BlueprintInternalUseOnly = "true"))
	static FBox Conv_IdentityBox(FBox Value);

	/** Identity function for FBox2D - used internally for literal value handling */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "Identity (Box2D)", BlueprintInternalUseOnly = "true"))
	static FBox2D Conv_IdentityBox2D(FBox2D Value);

	// Note: FSphere and FRay are not supported by Blueprint, so no identity functions for them

	//================ Vector/Struct Precision Conversion (Double -> Float) ========

	/** Converts FVector (double precision) to FVector3f (float precision) */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "To Vector3f (Vector)", CompactNodeTitle = "->", BlueprintAutocast))
	static FVector3f Conv_VectorToVector3f(FVector InVector);

	/** Converts FQuat (double precision) to FQuat4f (float precision) */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "To Quat4f (Quat)", CompactNodeTitle = "->", BlueprintAutocast))
	static FQuat4f Conv_QuatToQuat4f(FQuat InQuat);

	/** Converts FRotator (double precision) to FRotator3f (float precision) */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "To Rotator3f (Rotator)", CompactNodeTitle = "->", BlueprintAutocast))
	static FRotator3f Conv_RotatorToRotator3f(FRotator InRotator);

	/** Converts FTransform (double precision) to FTransform3f (float precision) */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "To Transform3f (Transform)", CompactNodeTitle = "->", BlueprintAutocast))
	static FTransform3f Conv_TransformToTransform3f(FTransform InTransform);

	/** Converts FVector2D (double precision) to FVector2f (float precision) */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "To Vector2f (Vector2D)", CompactNodeTitle = "->", BlueprintAutocast))
	static FVector2f Conv_Vector2DToVector2f(FVector2D InVector2D);

	/** Converts FVector4 (double precision) to FVector4f (float precision) */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "To Vector4f (Vector4)", CompactNodeTitle = "->", BlueprintAutocast))
	static FVector4f Conv_Vector4ToVector4f(FVector4 InVector4);

	/** Converts FMatrix (double precision) to FMatrix44f (float precision) */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "To Matrix44f (Matrix)", CompactNodeTitle = "->", BlueprintAutocast))
	static FMatrix44f Conv_MatrixToMatrix44f(FMatrix InMatrix);

	/** Converts FPlane (double precision) to FPlane4f (float precision) */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "To Plane4f (Plane)", CompactNodeTitle = "->", BlueprintAutocast))
	static FPlane4f Conv_PlaneToPlane4f(FPlane InPlane);

	/** Converts FBox (double precision) to FBox3f (float precision) */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "To Box3f (Box)", CompactNodeTitle = "->", BlueprintAutocast))
	static FBox3f Conv_BoxToBox3f(FBox InBox);

	/** Converts FBox2D (double precision) to FBox2f (float precision) */
	UFUNCTION(BlueprintPure, Category = "Magnus|Convert", meta = (DisplayName = "To Box2f (Box2D)", CompactNodeTitle = "->", BlueprintAutocast))
	static FBox2f Conv_Box2DToBox2f(FBox2D InBox2D);

	// Note: FSphere3f and FRay3f are not supported by Blueprint, so no conversion functions for them

};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
