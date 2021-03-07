// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ImGUIBPLibrary.generated.h"

DECLARE_DYNAMIC_DELEGATE(FOnVoidDelegate);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnBoolDelegate, bool, Value);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnFloatDelegate, float, Value);

/* 
*	Function library class.
*	Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.
*
*	When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.
*	BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.
*	BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.
*	DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.
*				Its lets you name the node using characters not allowed in C++ function names.
*	CompactNodeTitle - the word(s) that appear on the node.
*	Keywords -	the list of keywords that helps you to find node when you search for it using Blueprint drop-down menu. 
*				Good example is "Print String" node which you can find also by using keyword "log".
*	Category -	the category your node will be under in the Blueprint drop-down menu.
*
*	For more info on custom blueprint nodes visit documentation:
*	https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation
*/
UCLASS()
class UImGUIBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	// Demo, Debug, Information
	UFUNCTION(BlueprintCallable, Category = "ImGUI|Demo")
	static void ShowDemoWindow();

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Information")
    static void ShowMetricsWindow();

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Information")
    static void ShowAboutWindow();

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Style")
    static void ShowStyleEditor();

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Style")
    static void ShowStyleSelector(FString Label);

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Style")
    static void ShowFontSelector(FString Label);

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Information")
    static void ShowUserGuide();

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Information")
    static FString GetVersion();

	// Styles

	// Window

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Window")
    static bool Begin(FString Name,bool bOpen,int Flags);

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Window")
	static void Begin_Event(FString Name,bool bOpen,int Flags,const FOnVoidDelegate& OnEnabled);

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Window")
    static void End();

	// Child Window
	UFUNCTION(BlueprintCallable, Category = "ImGUI|Window|Child")
    static bool BeginChild(FString ID, FVector2D Size, bool bBorder, int Flags);

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Window|Child")
    static void BeginChild_Event(FString ID, FVector2D Size, bool bBorder, int Flags,const FOnVoidDelegate& OnEnabled);

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Window|Child")
    static bool BeginChild_IntID(int ID, FVector2D Size, bool bBorder, int Flags);

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Window|Child")
    static void BeginChild_IntID_Event(int ID, FVector2D Size, bool bBorder, int Flags,const FOnVoidDelegate& OnEnabled);

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Window|Child")
    static void EndChild();
	

	// Text

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Text")
    static void Text(FString Text);

	// FLoat

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Float")
    static bool InputFloat(FString Label,UPARAM(ref) float& Value,float Step,float StepFast,int FractionalDigits,int Flags);

	UFUNCTION(BlueprintCallable, Category = "ImGUI|Float")
    static void InputFloat_Event(FString Label,UPARAM(ref) float& Value,float Step,float StepFast,int FractionalDigits,int Flags,const FOnFloatDelegate& OnValueChanged);
};
