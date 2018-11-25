// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiInputHandler.h"

#include "ImGuiContextProxy.h"
#include "ImGuiModuleManager.h"
#include "ImGuiSettings.h"

#include <Engine/Console.h>
#include <Input/Events.h>

#if WITH_EDITOR
#include <Commands/InputBindingManager.h>
#include <Commands/InputChord.h>
#include <DebuggerCommands.h>
#endif // WITH_EDITOR


DEFINE_LOG_CATEGORY(LogImGuiInputHandler);


FImGuiInputResponse UImGuiInputHandler::OnKeyDown(const FKeyEvent& KeyEvent)
{
	// If this is an input mode switch event then handle it here and consume.
	if (IsToggleInputEvent(KeyEvent))
	{
		FImGuiModuleProperties::Get().ToggleInput();
		return FImGuiInputResponse().RequestConsume();
	}

	// Ignore console events, so we don't block it from opening.
	if (IsConsoleEvent(KeyEvent))
	{
		return FImGuiInputResponse{ false, false };
	}

#if WITH_EDITOR
	// If there is no active ImGui control that would get precedence and this key event is bound to a stop play session
	// command, then ignore that event and let the command execute.
	if (!HasImGuiActiveItem() && IsStopPlaySessionEvent(KeyEvent))
	{
		return FImGuiInputResponse{ false, false };
	}
#endif // WITH_EDITOR

	return DefaultResponse();
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

	bool AreModifiersMatching(const FImGuiKeyInfo& KeyInfo, const FKeyEvent& KeyEvent)
	{
		return IsMatching(KeyInfo.Shift, KeyEvent.IsShiftDown())
			&& IsMatching(KeyInfo.Ctrl, KeyEvent.IsControlDown())
			&& IsMatching(KeyInfo.Alt, KeyEvent.IsAltDown())
			&& IsMatching(KeyInfo.Cmd, KeyEvent.IsCommandDown());
	}
}

bool UImGuiInputHandler::IsToggleInputEvent(const FKeyEvent& KeyEvent) const
{
	return GImGuiSettings
		&& (KeyEvent.GetKey() == GImGuiSettings->GetToggleInputKey().Key)
		&& AreModifiersMatching(GImGuiSettings->GetToggleInputKey(), KeyEvent);
}

bool UImGuiInputHandler::HasImGuiActiveItem() const
{
	FImGuiContextProxy* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex);
	return ContextProxy && ContextProxy->HasActiveItem();
}

void UImGuiInputHandler::Initialize(FImGuiModuleManager* InModuleManager, UGameViewportClient* InGameViewport, int32 InContextIndex)
{
	ModuleManager = InModuleManager;
	GameViewport = InGameViewport;
	ContextIndex = InContextIndex;

#if WITH_EDITOR
	StopPlaySessionCommandInfo = FInputBindingManager::Get().FindCommandInContext("PlayWorld", "StopPlaySession");
	if (!StopPlaySessionCommandInfo.IsValid())
	{
		UE_LOG(LogImGuiInputHandler, Warning, TEXT("Couldn't find 'StopPlaySession' in context 'PlayWorld'. ")
			TEXT("PIE feature allowing execution of stop command in ImGui input mode will be disabled."));
	}
#endif // WITH_EDITOR
}
