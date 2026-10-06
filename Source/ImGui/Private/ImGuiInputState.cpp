// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiInputState.h"

#include <algorithm>

FImGuiInputState::FImGuiInputState()
{
	Reset();
}

void FImGuiInputState::AddCharacter(TCHAR Char)
{
	InputCharacters.Add(Char);
	FInputEvent& Event = InputEvents.AddDefaulted_GetRef();
	Event.Type = EEventType::Character;
	Event.Character = static_cast<uint32>(Char);
}

void FImGuiInputState::SetKeyDown(uint32 KeyIndex, bool bIsDown)
{
	if (KeyIndex < Utilities::GetArraySize(KeysDown) && KeysDown[KeyIndex] != bIsDown)
	{
		KeysDown[KeyIndex] = bIsDown;
		KeysUpdateRange.AddPosition(KeyIndex);
		FInputEvent& Event = InputEvents.AddDefaulted_GetRef();
		Event.Type = EEventType::Key;
		Event.Key = static_cast<ImGuiKey>(ImGuiKey_NamedKey_BEGIN + KeyIndex);
		Event.bIsDown = bIsDown;
	}
}

void FImGuiInputState::SetMouseDown(uint32 MouseIndex, bool bIsDown)
{
	if (MouseIndex < Utilities::GetArraySize(MouseButtonsDown) && MouseButtonsDown[MouseIndex] != bIsDown)
	{
		MouseButtonsDown[MouseIndex] = bIsDown;
		MouseButtonsUpdateRange.AddPosition(MouseIndex);
		FInputEvent& Event = InputEvents.AddDefaulted_GetRef();
		Event.Type = EEventType::MouseButton;
		Event.Button = static_cast<int>(MouseIndex);
		Event.bIsDown = bIsDown;
	}
}

void FImGuiInputState::QueuePointerPosition(const FVector2D& Position, ImGuiMouseSource Source)
{
	// ImGui deduplicates after source filtering. A locally cached event may never have reached IO.
	FInputEvent& Event = InputEvents.AddDefaulted_GetRef();
	Event.Type = EEventType::MousePosition;
	Event.X = static_cast<float>(Position.X);
	Event.Y = static_cast<float>(Position.Y);
	Event.MouseSource = Source;
}

void FImGuiInputState::SetMousePosition(const FVector2D& Position)
{
	MousePosition = Position;
	QueuePointerPosition(Position, ImGuiMouseSource_Mouse);
}

void FImGuiInputState::AddMouseWheelDelta(float DeltaValue)
{
	if (DeltaValue != 0.f)
	{
		MouseWheelDelta += DeltaValue;
		FInputEvent& Event = InputEvents.AddDefaulted_GetRef();
		Event.Type = EEventType::MouseWheel;
		Event.Y = DeltaValue;
	}
}

void FImGuiInputState::SetTouchPosition(const FVector2D& Position)
{
	TouchPosition = Position;
	QueuePointerPosition(Position, ImGuiMouseSource_TouchScreen);
}

void FImGuiInputState::SetTouchDown(bool bIsDown)
{
	if (bTouchDown != bIsDown)
	{
		bTouchDown = bIsDown;
		FInputEvent& Event = InputEvents.AddDefaulted_GetRef();
		Event.Type = EEventType::MouseButton;
		Event.Button = 0;
		Event.bIsDown = bIsDown;
		Event.MouseSource = ImGuiMouseSource_TouchScreen;
	}
}

void FImGuiInputState::SetModifierKey(bool& State, bool bIsDown, ImGuiKey Key)
{
	if (State != bIsDown)
	{
		State = bIsDown;
		FInputEvent& Event = InputEvents.AddDefaulted_GetRef();
		Event.Type = EEventType::Key;
		Event.Key = Key;
		Event.bIsDown = bIsDown;
	}
}

void FImGuiInputState::QueueNavigationChanges(const FNavInputArray& Previous)
{
	for (int Index = 0; Index < IM_ARRAYSIZE(NavigationInputs); ++Index)
	{
		if (NavigationInputs[Index] != Previous[Index])
		{
			FInputEvent& Event = InputEvents.AddDefaulted_GetRef();
			Event.Type = EEventType::Gamepad;
			Event.Key = static_cast<ImGuiKey>(ImGuiKey_GamepadStart + Index);
			Event.X = NavigationInputs[Index];
			Event.bIsDown = Event.X > 0.1f;
		}
	}
}

void FImGuiInputState::SetGamepadNavigationKey(const FKey& Key, bool bIsDown)
{
	FNavInputArray Previous;
	std::copy(NavigationInputs, NavigationInputs + IM_ARRAYSIZE(NavigationInputs), Previous);
	ImGuiInterops::SetGamepadNavigationKey(NavigationInputs, Key, bIsDown);
	QueueNavigationChanges(Previous);
}

void FImGuiInputState::SetGamepadNavigationAxis(const FKey& Key, float Value)
{
	FNavInputArray Previous;
	std::copy(NavigationInputs, NavigationInputs + IM_ARRAYSIZE(NavigationInputs), Previous);
	ImGuiInterops::SetGamepadNavigationAxis(NavigationInputs, Key, Value);
	QueueNavigationChanges(Previous);
}

void FImGuiInputState::SetGamepadNavigationEnabled(bool bEnabled)
{
	if (bGamepadNavigationEnabled && !bEnabled) ResetGamepadNavigation();
	bGamepadNavigationEnabled = bEnabled;
}

void FImGuiInputState::SetGamepad(bool bInHasGamepad)
{
	if (bHasGamepad && !bInHasGamepad) ResetGamepadNavigation();
	if (!bHasGamepad && bInHasGamepad)
	{
		// Recover held state even if connection confirmation follows an earlier input copy.
		const FNavInputArray Empty = {};
		QueueNavigationChanges(Empty);
	}
	bHasGamepad = bInHasGamepad;
}

void FImGuiInputState::SetFocused(bool bIsFocused)
{
	if (bFocused != bIsFocused)
	{
		bFocused = bIsFocused;
		if (!bFocused) Reset();
		FInputEvent& Event = InputEvents.AddDefaulted_GetRef();
		Event.Type = EEventType::Focus;
		Event.bIsDown = bFocused;
	}
}

void FImGuiInputState::Reset()
{
	InputEvents.Reset();
	ResetKeyboard();
	ResetMouse();
	ResetGamepadNavigation();
}

void FImGuiInputState::ResetKeyboard()
{
	InputEvents.RemoveAll([](const FInputEvent& Event) { return Event.Type == EEventType::Key || Event.Type == EEventType::Character; });
	ClearCharacters();
	std::fill(KeysDown, KeysDown + IM_ARRAYSIZE(KeysDown), false);
	bIsControlDown = bIsShiftDown = bIsAltDown = bIsSuperDown = false;
	KeysUpdateRange.SetFull();
	bResetKeyboard = true;
}

void FImGuiInputState::ResetMouse()
{
	InputEvents.RemoveAll([](const FInputEvent& Event)
	{
		return Event.Type == EEventType::MousePosition || Event.Type == EEventType::MouseButton || Event.Type == EEventType::MouseWheel;
	});
	std::fill(MouseButtonsDown, MouseButtonsDown + IM_ARRAYSIZE(MouseButtonsDown), false);
	MouseButtonsUpdateRange.SetFull();
	MouseWheelDelta = 0.f;
	MousePosition = TouchPosition = FVector2D(-FLT_MAX, -FLT_MAX);
	bTouchDown = bTouchProcessed = false;
	bResetMouse = true;
}

void FImGuiInputState::ResetGamepadNavigation()
{
	InputEvents.RemoveAll([](const FInputEvent& Event) { return Event.Type == EEventType::Gamepad; });
	std::fill(NavigationInputs, NavigationInputs + IM_ARRAYSIZE(NavigationInputs), 0.f);
	bResetGamepad = true;
}

void FImGuiInputState::ClearCharacters()
{
	InputCharacters.Reset();
}

void FImGuiInputState::ClearUpdateState()
{
	ClearCharacters();
	InputEvents.Reset();
	KeysUpdateRange.SetEmpty();
	MouseButtonsUpdateRange.SetEmpty();
	MouseWheelDelta = 0.f;
	bTouchProcessed = bTouchDown;
	bResetKeyboard = bResetMouse = bResetGamepad = false;
}

