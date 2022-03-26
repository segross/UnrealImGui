#pragma once

#include "ImGuiModuleManager.h"
#include "Framework/Application/IInputProcessor.h"

class FImGuiModuleManager;

class FImGuiInputPreprocessor : public IInputProcessor
{
public:
	FImGuiInputPreprocessor(FImGuiModuleManager& InModuleManager)
		: ModuleManager(InModuleManager)
	{
	}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
	{
	}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		return false;
	}

	virtual bool
	HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override
	{
		return false;
	}

	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& InPointerEvent) override
	{
		const bool bIsWindowHovered = ImGui::IsWindowHovered();
		FImGuiContextProxy* Proxy = ModuleManager.GetContextManager().GetContextProxy(0);
		if (Proxy)
		{
			GEngine->AddOnScreenDebugMessage(10, 0, Proxy->WantsMouseCapture() ? FColor::Green : FColor::Red, TEXT("Move Capture"));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(10, 0, FColor::Red, TEXT("No proxy"));
		}
		
		GEngine->AddOnScreenDebugMessage(1, 0, bIsWindowHovered ? FColor::Green : FColor::Red, TEXT("Move Hover"));
		return false;
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& InPointerEvent) override
	{
		//if (ModuleManager.GetProperties().IsMouseInputShared())
		{
			const bool bIsWindowHovered = ImGui::IsWindowHovered();
			GEngine->AddOnScreenDebugMessage(2, 10, bIsWindowHovered ? FColor::Green : FColor::Red, TEXT("Click Hover"));
			return bIsWindowHovered;
		}
		return false;
	}

	virtual bool
	HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& InPointerEvent) override
	{
		return false;
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("CommonInput"); }

private:
	bool IsRelevantInput(FSlateApplication& SlateApp, const FInputEvent& InputEvent)
	{
		return false;
	}

	FImGuiModuleManager& ModuleManager;
};
