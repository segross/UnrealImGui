// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGUIBPLibrary.h"
#include "ImGUIBlueprintLibrary.h"
#include "Converter.h"
#include "ImGuiCommon.h"

//Exemples on
//	ImGui::ShowDemoWindow()

UImGUIBPLibrary::UImGUIBPLibrary(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	
}

void UImGUIBPLibrary::ShowDemoWindow()
{
#if WITH_IMGUI
	ImGui::ShowDemoWindow();
#endif
}

void UImGUIBPLibrary::ShowMetricsWindow()
{
#if WITH_IMGUI
	ImGui::ShowMetricsWindow();
#endif
}

void UImGUIBPLibrary::ShowAboutWindow()
{
#if WITH_IMGUI
	ImGui::ShowAboutWindow();
#endif
}

void UImGUIBPLibrary::ShowStyleEditor()
{
#if WITH_IMGUI
	ImGui::ShowStyleEditor();
#endif
}

void UImGUIBPLibrary::ShowStyleSelector(FString Label)
{
#if WITH_IMGUI
	ImGui::ShowStyleSelector(TCHAR_TO_ANSI(*Label));
#endif
}

void UImGUIBPLibrary::ShowFontSelector(FString Label)
{
#if WITH_IMGUI
	ImGui::ShowFontSelector(TCHAR_TO_ANSI(*Label));
#endif
}

void UImGUIBPLibrary::ShowUserGuide()
{
#if WITH_IMGUI
	ImGui::ShowUserGuide();
#endif
}

FString UImGUIBPLibrary::GetVersion()
{
	FString Version;
#if WITH_IMGUI
	Version = ImGui::GetVersion();
#endif
	return Version;
}

bool UImGUIBPLibrary::Begin(FString Name,bool bOpen,int Flags)
{
	bool ReturnValue = false;
#if WITH_IMGUI
	ReturnValue = ImGui::Begin(TCHAR_TO_ANSI(*Name),&bOpen,Flags);
	
#endif
	return ReturnValue;
}

void UImGUIBPLibrary::Begin_Event(FString Name,bool bOpen,int Flags, const FOnVoidDelegate& OnEnabled)
{
	if (Begin(Name,bOpen,Flags) && OnEnabled.IsBound())
	{
		OnEnabled.Execute();
	}
	End();
}

void UImGUIBPLibrary::End()
{
#if WITH_IMGUI
	ImGui::End();
#endif
}

bool UImGUIBPLibrary::BeginChild(FString ID, FVector2D Size, bool bBorder, int Flags)
{
	bool ReturnValue = false;
#if WITH_IMGUI
	ReturnValue = ImGui::BeginChild(TCHAR_TO_ANSI(*ID),Converter::FVector2DToImVec2(Size),bBorder,Flags);
#endif
	return ReturnValue;
}

void UImGUIBPLibrary::BeginChild_Event(FString ID, FVector2D Size, bool bBorder, int Flags,const FOnVoidDelegate& OnEnabled)
{
	if (BeginChild(ID,Size,bBorder,Flags) && OnEnabled.IsBound())
	{
		OnEnabled.Execute();
	}
}

bool UImGUIBPLibrary::BeginChild_IntID(int ID, FVector2D Size, bool bBorder, int Flags)
{
	bool ReturnValue = false;
#if WITH_IMGUI
	ReturnValue = ImGui::BeginChild(ID,Converter::FVector2DToImVec2(Size),bBorder,Flags);
#endif
	return ReturnValue;
}

void UImGUIBPLibrary::BeginChild_IntID_Event(int ID, FVector2D Size, bool bBorder, int Flags,const FOnVoidDelegate& OnEnabled)
{
	if (BeginChild_IntID(ID,Size,bBorder,Flags) && OnEnabled.IsBound())
	{
		OnEnabled.Execute();
	}
	EndChild();
}

void UImGUIBPLibrary::EndChild()
{
#if WITH_IMGUI
	ImGui::EndChild();
#endif
}

void UImGUIBPLibrary::Text(FString Text)
{
#if WITH_IMGUI
	ImGui::Text(TCHAR_TO_ANSI(*Text));
#endif
}

bool UImGUIBPLibrary::InputFloat(FString Label,float& Value,float Step,float StepFast,int FractionalDigits,int Flags)
{
	bool ReturnValue = false;
#if WITH_IMGUI
	ReturnValue = ImGui::InputFloat(TCHAR_TO_ANSI(*Label),&Value,Step,StepFast,Converter::FloatFormat(FractionalDigits),Flags);
#endif
	return ReturnValue;
}

void UImGUIBPLibrary::InputFloat_Event(FString Label, float& Value, float Step, float StepFast,int FractionalDigits, int Flags, const FOnFloatDelegate& OnValueChanged)
{
	if (InputFloat(Label,Value,Step,StepFast,FractionalDigits,Flags) && OnValueChanged.IsBound())
	{
		OnValueChanged.Execute(Value);
	}
}



