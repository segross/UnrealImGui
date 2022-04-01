// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiInputHandler.h"

#include "ImGuiContextProxy.h"
#include "ImGuiInputState.h"
#include "ImGuiModuleDebug.h"
#include "ImGuiModuleManager.h"
#include "ImGuiModuleSettings.h"
#include "VersionCompatibility.h"

#include <Engine/Console.h>
#include <Framework/Application/SlateApplication.h>
#include <GameFramework/InputSettings.h>
#include <InputCoreTypes.h>
#include <Input/Events.h>

#if WITH_EDITOR
#include <Framework/Commands/InputBindingManager.h>
#include <Framework/Commands/InputChord.h>
#include <Kismet2/DebuggerCommands.h>
#endif // WITH_EDITOR


DEFINE_LOG_CATEGORY(LogImGuiInputHandler);

namespace
{
	FReply ToReply(bool bConsume)
	{
		return bConsume ? FReply::Handled() : FReply::Unhandled();
	}
}

FReply UImGuiInputHandler::OnKeyChar(const struct FCharacterEvent& CharacterEvent)
{
	InputState->AddCharacter(CharacterEvent.GetCharacter());
	return ToReply(!ModuleManager->GetProperties().IsKeyboardInputShared());
}

FReply UImGuiInputHandler::OnKeyDown(const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey().IsGamepadKey())
	{
		bool bConsume = false;
		if (InputState->IsGamepadNavigationEnabled())
		{
			InputState->SetGamepadNavigationKey(KeyEvent, true);
			bConsume = !ModuleManager->GetProperties().IsGamepadInputShared();
		}

		return ToReply(bConsume);
	}
	else
	{
		// Ignore console events, so we don't block it from opening.
		if (IsConsoleEvent(KeyEvent))
		{
			return ToReply(false);
		}

#if WITH_EDITOR
		// If there is no active ImGui control that would get precedence and this key event is bound to a stop play session
		// command, then ignore that event and let the command execute.
		if (!HasImGuiActiveItem() && IsStopPlaySessionEvent(KeyEvent))
		{
			return ToReply(false);
		}
#endif // WITH_EDITOR

		const bool bConsume = !ModuleManager->GetProperties().IsKeyboardInputShared();

		// With shared input we can leave command bindings for DebugExec to handle, otherwise we need to do it here.
		if (bConsume && IsToggleInputEvent(KeyEvent))
		{
			ModuleManager->GetProperties().ToggleInput();
		}

		InputState->SetKeyDown(KeyEvent, true);
		CopyModifierKeys(KeyEvent);

		InputState->KeyDownEvents.Add(KeyEvent.GetKeyCode(), KeyEvent);

		return ToReply(bConsume);
	}
}

FReply UImGuiInputHandler::OnKeyUp(const FKeyEvent& KeyEvent)
{
	InputState->KeyUpEvents.Add(KeyEvent.GetKeyCode(), KeyEvent);
	
	if (KeyEvent.GetKey().IsGamepadKey())
	{
		bool bConsume = false;
		if (InputState->IsGamepadNavigationEnabled())
		{
			InputState->SetGamepadNavigationKey(KeyEvent, false);
			bConsume = !ModuleManager->GetProperties().IsGamepadInputShared();
		}

		return ToReply(bConsume);
	}
	else
	{
		InputState->SetKeyDown(KeyEvent, false);
		CopyModifierKeys(KeyEvent);

		return ToReply(!ModuleManager->GetProperties().IsKeyboardInputShared());
	}
}

FReply UImGuiInputHandler::OnAnalogValueChanged(const FAnalogInputEvent& AnalogInputEvent)
{
	bool bConsume = false;

	if (AnalogInputEvent.GetKey().IsGamepadKey() && InputState->IsGamepadNavigationEnabled())
	{
		InputState->SetGamepadNavigationAxis(AnalogInputEvent, AnalogInputEvent.GetAnalogValue());
		bConsume = !ModuleManager->GetProperties().IsGamepadInputShared();
	}

	return ToReply(bConsume);
}

FReply UImGuiInputHandler::OnMouseButtonDown(const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsTouchEvent())
	{
		return ToReply(false);
	}

	InputState->SetMouseDown(MouseEvent, true);
	if (ModuleManager)
	{
		FImGuiContextProxy* Proxy = ModuleManager->GetContextManager().GetContextProxy(0);
		if (Proxy)
		{
			GEngine->AddOnScreenDebugMessage(15, 10, Proxy->WantsMouseCapture() ? FColor::Green : FColor::Red, TEXT("Handler Down"));
			return ToReply(Proxy->WantsMouseCapture());
		}
	}
	return ToReply(true);
}

FReply UImGuiInputHandler::OnMouseButtonDoubleClick(const FPointerEvent& MouseEvent)
{
	InputState->SetMouseDown(MouseEvent, true);
	return ToReply(true);
}

FReply UImGuiInputHandler::OnMouseButtonUp(const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsTouchEvent())
	{
		return ToReply(false);
	}

	InputState->SetMouseDown(MouseEvent, false);
	return ToReply(true);
}

FReply UImGuiInputHandler::OnMouseWheel(const FPointerEvent& MouseEvent)
{
	InputState->AddMouseWheelDelta(MouseEvent.GetWheelDelta());
	return ToReply(true);
}

FReply UImGuiInputHandler::OnMouseMove(const FVector2D& MousePosition, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsTouchEvent())
	{
		return ToReply(false);
	}

	return OnMouseMove(MousePosition);
}

FReply UImGuiInputHandler::OnMouseMove(const FVector2D& MousePosition)
{
	InputState->SetMousePosition(MousePosition);
	return ToReply(true);
}

FReply UImGuiInputHandler::OnTouchStarted(const FVector2D& CursorPosition, const FPointerEvent& TouchEvent)
{
	InputState->SetTouchDown(true);
	InputState->SetTouchPosition(CursorPosition);
	return ToReply(true);
}

FReply UImGuiInputHandler::OnTouchMoved(const FVector2D& CursorPosition, const FPointerEvent& TouchEvent)
{
	InputState->SetTouchPosition(CursorPosition);
	return ToReply(true);
}

FReply UImGuiInputHandler::OnTouchEnded(const FVector2D& CursorPosition, const FPointerEvent& TouchEvent)
{
	InputState->SetTouchDown(false);
	return ToReply(true);
}

void UImGuiInputHandler::OnKeyboardInputEnabled()
{
	bKeyboardInputEnabled = true;
}

void UImGuiInputHandler::OnKeyboardInputDisabled()
{
	if (bKeyboardInputEnabled)
	{
		bKeyboardInputEnabled = false;
		InputState->ResetKeyboard();
	}
}

void UImGuiInputHandler::OnGamepadInputEnabled()
{
	bGamepadInputEnabled = true;
}

void UImGuiInputHandler::OnGamepadInputDisabled()
{
	if (bGamepadInputEnabled)
	{
		bGamepadInputEnabled = false;
		InputState->ResetGamepadNavigation();
	}
}

void UImGuiInputHandler::OnMouseInputEnabled()
{
	if (!bMouseInputEnabled)
	{
		bMouseInputEnabled = true;
		UpdateInputStatePointer();
	}
}

void UImGuiInputHandler::OnMouseInputDisabled()
{
	if (bMouseInputEnabled)
	{
		bMouseInputEnabled = false;
		InputState->ResetMouse();
		UpdateInputStatePointer();
	}
}

void UImGuiInputHandler::CopyModifierKeys(const FInputEvent& InputEvent)
{
	InputState->SetControlDown(InputEvent.IsControlDown());
	InputState->SetShiftDown(InputEvent.IsShiftDown());
	InputState->SetAltDown(InputEvent.IsAltDown());
}

bool UImGuiInputHandler::IsConsoleEvent(const FKeyEvent& KeyEvent) const
{
	// Checking modifiers is based on console implementation.
	const bool bModifierDown = KeyEvent.IsControlDown() || KeyEvent.IsShiftDown() || KeyEvent.IsAltDown() || KeyEvent.IsCommandDown();
	return !bModifierDown && GetDefault<UInputSettings>()->ConsoleKeys.Contains(KeyEvent.GetKey());
}

#if WITH_EDITOR
bool UImGuiInputHandler::IsStopPlaySessionEvent(const FKeyEvent& KeyEvent) const
{
	if (StopPlaySessionCommandInfo.IsValid())
	{
		const FInputChord InputChord(KeyEvent.GetKey(), KeyEvent.IsShiftDown(), KeyEvent.IsControlDown(), KeyEvent.IsAltDown(), KeyEvent.IsCommandDown());
#if ENGINE_COMPATIBILITY_SINGLE_KEY_BINDING
		const bool bHasActiveChord = (InputChord == StopPlaySessionCommandInfo->GetActiveChord().Get());
#else
		const bool bHasActiveChord = StopPlaySessionCommandInfo->HasActiveChord(InputChord);
#endif
		return bHasActiveChord && FPlayWorldCommands::GlobalPlayWorldActions->CanExecuteAction(StopPlaySessionCommandInfo.ToSharedRef());
	}

	return false;
}
#endif // WITH_EDITOR

namespace
{
	bool IsMatching(ECheckBoxState CheckBoxState, bool bValue)
	{
		return (CheckBoxState == ECheckBoxState::Undetermined) || ((CheckBoxState == ECheckBoxState::Checked) == bValue);
	}

	bool IsMatchingEvent(const FKeyEvent& KeyEvent, const FImGuiKeyInfo& KeyInfo)
	{
		return (KeyInfo.Key == KeyEvent.GetKey())
			&& IsMatching(KeyInfo.Shift, KeyEvent.IsShiftDown())
			&& IsMatching(KeyInfo.Ctrl, KeyEvent.IsControlDown())
			&& IsMatching(KeyInfo.Alt, KeyEvent.IsAltDown())
			&& IsMatching(KeyInfo.Cmd, KeyEvent.IsCommandDown());
	}
}

bool UImGuiInputHandler::IsToggleInputEvent(const FKeyEvent& KeyEvent) const
{
	return IsMatchingEvent(KeyEvent, ModuleManager->GetSettings().GetToggleInputKey());
}

bool UImGuiInputHandler::HasImGuiActiveItem() const
{
	FImGuiContextProxy* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex);
	return ContextProxy && ContextProxy->HasActiveItem();
}

void UImGuiInputHandler::UpdateInputStatePointer()
{
	InputState->SetMousePointer(bMouseInputEnabled && ModuleManager->GetSettings().UseSoftwareCursor());
}

void UImGuiInputHandler::OnSoftwareCursorChanged(bool)
{
	UpdateInputStatePointer();
}

void UImGuiInputHandler::OnPostImGuiUpdate()
{
	InputState->ClearUpdateState();

	// TODO Replace with delegates after adding property change events.
	InputState->SetKeyboardNavigationEnabled(ModuleManager->GetProperties().IsKeyboardNavigationEnabled());
	InputState->SetGamepadNavigationEnabled(ModuleManager->GetProperties().IsGamepadNavigationEnabled());

	const auto& PlatformApplication = FSlateApplication::Get().GetPlatformApplication();
	InputState->SetGamepad(PlatformApplication.IsValid() && PlatformApplication->IsGamepadAttached());
}

void UImGuiInputHandler::Initialize(FImGuiModuleManager* InModuleManager, UGameViewportClient* InGameViewport, int32 InContextIndex)
{
	ModuleManager = InModuleManager;
	GameViewport = InGameViewport;
	ContextIndex = InContextIndex;

	auto* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex);
	checkf(ContextProxy, TEXT("Missing context during initialization of input handler: ContextIndex = %d"), ContextIndex);
	InputState = &ContextProxy->GetInputState();

	// Register to get post-update notifications, so we can clean frame updates.
	ModuleManager->OnPostImGuiUpdate().AddUObject(this, &UImGuiInputHandler::OnPostImGuiUpdate);

	auto& Settings = ModuleManager->GetSettings();
	if (!Settings.OnUseSoftwareCursorChanged.IsBoundToObject(this))
	{
		Settings.OnUseSoftwareCursorChanged.AddUObject(this, &UImGuiInputHandler::OnSoftwareCursorChanged);
	}

#if WITH_EDITOR
	StopPlaySessionCommandInfo = FInputBindingManager::Get().FindCommandInContext("PlayWorld", "StopPlaySession");
	if (!StopPlaySessionCommandInfo.IsValid())
	{
		UE_LOG(LogImGuiInputHandler, Warning, TEXT("Couldn't find 'StopPlaySession' in context 'PlayWorld'. ")
			TEXT("PIE feature allowing execution of stop command in ImGui input mode will be disabled."));
	}
#endif // WITH_EDITOR
}

void UImGuiInputHandler::BeginDestroy()
{
	Super::BeginDestroy();

	// To catch leftovers from modules shutdown during PIE session.
	extern FImGuiModuleManager* ImGuiModuleManager;
	if (ModuleManager && ModuleManager == ImGuiModuleManager)
	{
		ModuleManager->GetSettings().OnUseSoftwareCursorChanged.RemoveAll(this);
	}
}

