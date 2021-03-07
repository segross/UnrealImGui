// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ImGuiCommon.h"

/**
 * 
 */
class IMGUIBLUEPRINTLIBRARY_API Converter
{
public:
	Converter();

	// Vector

	static FVector2D Float2ToFVector2D(int Value[2]);

	static void FVector2DToFloat2(FVector2D Value,float* Result[2]);

	static FVector Float3ToFVector(int Value[3]);

	static FVector4 Float4ToFVector4(int Value[4]);

	static FVector2D ImVec2ToFVector2D(ImVec2 Value);

	static ImVec2 FVector2DToImVec2(FVector2D Value);

	static FVector4 ImVec4ToFVector4(ImVec4 Value);

	static ImVec4 FVector4ToImVec4(FVector4 Value);
	

	// Rotator

	static FRotator Float3ToFRotator(int Value[3]);

	// Color

	static FColor Float3ToFColor(int Value[3]);

	static FColor Float4ToFColor(int Value[4]);

	static FColor ImVec4ToFColor(ImVec4 Value);
	

	static FLinearColor Float3ToFLinearColor(int Value[3]);

	static FLinearColor Float4ToFLinearColor(int Value[4]);

	static FLinearColor ImVec4ToFLinearColor(ImVec4 Value);


	static const char* FloatFormat(const int FractionalDigits);

	
	
};
