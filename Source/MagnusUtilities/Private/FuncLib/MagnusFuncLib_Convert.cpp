/*
* Magnus Utilities
* Author: Ember, All Rights Reserved.
*/

#include "FuncLib/MagnusFuncLib_Convert.h"

//———————— VariableConvert_DoubleToFloat																		————

float UMagnusFuncLib_Convert::VariableConvert_DoubleToFloat(double Value)
{
	return static_cast<float>(Value);
}

//———————— VariableConvert_FloatToDouble																		————

double UMagnusFuncLib_Convert::VariableConvert_FloatToDouble(float Value)
{
	return static_cast<double>(Value);
}

//———————— Conv_IdentityDouble																					————

double UMagnusFuncLib_Convert::Conv_IdentityDouble(double Value)
{
	return Value;
}

//———————— Conv_IdentityFloat																					————

float UMagnusFuncLib_Convert::Conv_IdentityFloat(float Value)
{
	return Value;
}

//================ Struct Identity (for literal value handling) ========

FVector2D UMagnusFuncLib_Convert::Conv_IdentityVector2D(FVector2D Value)
{
	return Value;
}

FVector UMagnusFuncLib_Convert::Conv_IdentityVector(FVector Value)
{
	return Value;
}

FVector4 UMagnusFuncLib_Convert::Conv_IdentityVector4(FVector4 Value)
{
	return Value;
}

FRotator UMagnusFuncLib_Convert::Conv_IdentityRotator(FRotator Value)
{
	return Value;
}

FQuat UMagnusFuncLib_Convert::Conv_IdentityQuat(FQuat Value)
{
	return Value;
}

FTransform UMagnusFuncLib_Convert::Conv_IdentityTransform(FTransform Value)
{
	return Value;
}

FMatrix UMagnusFuncLib_Convert::Conv_IdentityMatrix(FMatrix Value)
{
	return Value;
}

FPlane UMagnusFuncLib_Convert::Conv_IdentityPlane(FPlane Value)
{
	return Value;
}

FBox UMagnusFuncLib_Convert::Conv_IdentityBox(FBox Value)
{
	return Value;
}

FBox2D UMagnusFuncLib_Convert::Conv_IdentityBox2D(FBox2D Value)
{
	return Value;
}

// Note: FSphere and FRay are not supported by Blueprint, so no identity functions for them

//================ Vector/Struct Precision Conversion (Double -> Float) ========

FVector3f UMagnusFuncLib_Convert::Conv_VectorToVector3f(FVector InVector)
{
	return FVector3f(InVector);
}

FQuat4f UMagnusFuncLib_Convert::Conv_QuatToQuat4f(FQuat InQuat)
{
	return FQuat4f(InQuat);
}

FRotator3f UMagnusFuncLib_Convert::Conv_RotatorToRotator3f(FRotator InRotator)
{
	return FRotator3f(InRotator);
}

FTransform3f UMagnusFuncLib_Convert::Conv_TransformToTransform3f(FTransform InTransform)
{
	return FTransform3f(InTransform);
}

FVector2f UMagnusFuncLib_Convert::Conv_Vector2DToVector2f(FVector2D InVector2D)
{
	return FVector2f(InVector2D);
}

FVector4f UMagnusFuncLib_Convert::Conv_Vector4ToVector4f(FVector4 InVector4)
{
	return FVector4f(InVector4);
}

FMatrix44f UMagnusFuncLib_Convert::Conv_MatrixToMatrix44f(FMatrix InMatrix)
{
	return FMatrix44f(InMatrix);
}

FPlane4f UMagnusFuncLib_Convert::Conv_PlaneToPlane4f(FPlane InPlane)
{
	return FPlane4f(InPlane);
}

FBox3f UMagnusFuncLib_Convert::Conv_BoxToBox3f(FBox InBox)
{
	return FBox3f(FVector3f(InBox.Min), FVector3f(InBox.Max));
}

FBox2f UMagnusFuncLib_Convert::Conv_Box2DToBox2f(FBox2D InBox2D)
{
	return FBox2f(FVector2f(InBox2D.Min), FVector2f(InBox2D.Max));
}

// Note: FSphere3f and FRay3f are not supported by Blueprint, so no conversion functions for them

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
