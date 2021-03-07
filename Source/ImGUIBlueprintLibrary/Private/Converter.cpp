// Fill out your copyright notice in the Description page of Project Settings.


#include "Converter.h"

Converter::Converter()
{
}

FVector2D Converter::Float2ToFVector2D(int Value[2])
{
	return FVector2D(Value[0],Value[1]);
}

void Converter::FVector2DToFloat2(FVector2D Value,float* Result[2])
{
	Result[0] = &Value.X;
	Result[1] = &Value.Y;
}

FVector Converter::Float3ToFVector(int Value[3])
{
	return FVector(Value[0],Value[1],Value[2]);
}

FVector4 Converter::Float4ToFVector4(int Value[4])
{
	return FVector4(Value[0],Value[1],Value[2],Value[3]);
}

FVector2D Converter::ImVec2ToFVector2D(ImVec2 Value)
{
	return FVector2D(Value.x,Value.y);
}

ImVec2 Converter::FVector2DToImVec2(FVector2D Value)
{
	return ImVec2(Value.X,Value.Y);
}

FVector4 Converter::ImVec4ToFVector4(ImVec4 Value)
{
	return FVector4(Value.x,Value.y,Value.z,Value.w);
}

ImVec4 Converter::FVector4ToImVec4(FVector4 Value)
{
	return ImVec4(Value.X,Value.Y,Value.Z,Value.W);
}

FRotator Converter::Float3ToFRotator(int Value[3])
{
	return FRotator(Value[0],Value[1],Value[2]);
}

FColor Converter::Float3ToFColor(int Value[3])
{
	return FColor(Value[0],Value[1],Value[2]);
}

FColor Converter::Float4ToFColor(int Value[4])
{
	return FColor(Value[0],Value[1],Value[2],Value[3]);
}

FColor Converter::ImVec4ToFColor(ImVec4 Value)
{
	return FColor(Value.x,Value.y,Value.z,Value.w);
}

FLinearColor Converter::Float3ToFLinearColor(int Value[3])
{
	return FLinearColor(Value[0],Value[1],Value[2]);
}

FLinearColor Converter::Float4ToFLinearColor(int Value[4])
{
	return FLinearColor(Value[0],Value[1],Value[2],Value[3]);
}

FLinearColor Converter::ImVec4ToFLinearColor(ImVec4 Value)
{
	return FLinearColor(Value.x,Value.y,Value.z,Value.w);
}

const char* Converter::FloatFormat(const int FractionalDigits)
{
	FString Format = "%.";
	Format.AppendInt(FractionalDigits);
	Format += "f";

	return TCHAR_TO_ANSI(*Format);
}
