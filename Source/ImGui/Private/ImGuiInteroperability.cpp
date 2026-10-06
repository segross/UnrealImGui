// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiInteroperability.h"
#include "ImGuiInputState.h"
#include "ImGuiImplementation.h"

namespace ImGuiInterops
{
	ImGuiKey ToImGuiKey(const FKey& Key)
	{
		static const TMap<FKey, ImGuiKey> KeyMap = {
			{ EKeys::Tab, ImGuiKey_Tab },
			{ EKeys::Left, ImGuiKey_LeftArrow },
			{ EKeys::Right, ImGuiKey_RightArrow },
			{ EKeys::Up, ImGuiKey_UpArrow },
			{ EKeys::Down, ImGuiKey_DownArrow },
			{ EKeys::PageUp, ImGuiKey_PageUp },
			{ EKeys::PageDown, ImGuiKey_PageDown },
			{ EKeys::Home, ImGuiKey_Home },
			{ EKeys::End, ImGuiKey_End },
			{ EKeys::Insert, ImGuiKey_Insert },
			{ EKeys::Delete, ImGuiKey_Delete },
			{ EKeys::BackSpace, ImGuiKey_Backspace },
			{ EKeys::SpaceBar, ImGuiKey_Space },
			{ EKeys::Enter, ImGuiKey_Enter },
			{ EKeys::Escape, ImGuiKey_Escape },
			{ EKeys::Apostrophe, ImGuiKey_Apostrophe },
			{ EKeys::Comma, ImGuiKey_Comma },
			{ EKeys::Hyphen, ImGuiKey_Minus },
			{ EKeys::Period, ImGuiKey_Period },
			{ EKeys::Slash, ImGuiKey_Slash },
			{ EKeys::Semicolon, ImGuiKey_Semicolon },
			{ EKeys::Equals, ImGuiKey_Equal },
			{ EKeys::LeftBracket, ImGuiKey_LeftBracket },
			{ EKeys::Backslash, ImGuiKey_Backslash },
			{ EKeys::RightBracket, ImGuiKey_RightBracket },
			{ EKeys::Tilde, ImGuiKey_GraveAccent },
			{ EKeys::CapsLock, ImGuiKey_CapsLock },
			{ EKeys::ScrollLock, ImGuiKey_ScrollLock },
			{ EKeys::NumLock, ImGuiKey_NumLock },
			{ EKeys::Pause, ImGuiKey_Pause },
			{ EKeys::LeftShift, ImGuiKey_LeftShift },
			{ EKeys::RightShift, ImGuiKey_RightShift },
			{ EKeys::LeftControl, ImGuiKey_LeftCtrl },
			{ EKeys::RightControl, ImGuiKey_RightCtrl },
			{ EKeys::LeftAlt, ImGuiKey_LeftAlt },
			{ EKeys::RightAlt, ImGuiKey_RightAlt },
			{ EKeys::LeftCommand, ImGuiKey_LeftSuper },
			{ EKeys::RightCommand, ImGuiKey_RightSuper },
			{ EKeys::Decimal, ImGuiKey_KeypadDecimal },
			{ EKeys::Divide, ImGuiKey_KeypadDivide },
			{ EKeys::Multiply, ImGuiKey_KeypadMultiply },
			{ EKeys::Subtract, ImGuiKey_KeypadSubtract },
			{ EKeys::Add, ImGuiKey_KeypadAdd },
			{ EKeys::A, ImGuiKey_A },
			{ EKeys::B, ImGuiKey_B },
			{ EKeys::C, ImGuiKey_C },
			{ EKeys::D, ImGuiKey_D },
			{ EKeys::E, ImGuiKey_E },
			{ EKeys::F, ImGuiKey_F },
			{ EKeys::G, ImGuiKey_G },
			{ EKeys::H, ImGuiKey_H },
			{ EKeys::I, ImGuiKey_I },
			{ EKeys::J, ImGuiKey_J },
			{ EKeys::K, ImGuiKey_K },
			{ EKeys::L, ImGuiKey_L },
			{ EKeys::M, ImGuiKey_M },
			{ EKeys::N, ImGuiKey_N },
			{ EKeys::O, ImGuiKey_O },
			{ EKeys::P, ImGuiKey_P },
			{ EKeys::Q, ImGuiKey_Q },
			{ EKeys::R, ImGuiKey_R },
			{ EKeys::S, ImGuiKey_S },
			{ EKeys::T, ImGuiKey_T },
			{ EKeys::U, ImGuiKey_U },
			{ EKeys::V, ImGuiKey_V },
			{ EKeys::W, ImGuiKey_W },
			{ EKeys::X, ImGuiKey_X },
			{ EKeys::Y, ImGuiKey_Y },
			{ EKeys::Z, ImGuiKey_Z },
			{ EKeys::Zero, ImGuiKey_0 },
			{ EKeys::NumPadZero, ImGuiKey_Keypad0 },
			{ EKeys::One, ImGuiKey_1 },
			{ EKeys::NumPadOne, ImGuiKey_Keypad1 },
			{ EKeys::Two, ImGuiKey_2 },
			{ EKeys::NumPadTwo, ImGuiKey_Keypad2 },
			{ EKeys::Three, ImGuiKey_3 },
			{ EKeys::NumPadThree, ImGuiKey_Keypad3 },
			{ EKeys::Four, ImGuiKey_4 },
			{ EKeys::NumPadFour, ImGuiKey_Keypad4 },
			{ EKeys::Five, ImGuiKey_5 },
			{ EKeys::NumPadFive, ImGuiKey_Keypad5 },
			{ EKeys::Six, ImGuiKey_6 },
			{ EKeys::NumPadSix, ImGuiKey_Keypad6 },
			{ EKeys::Seven, ImGuiKey_7 },
			{ EKeys::NumPadSeven, ImGuiKey_Keypad7 },
			{ EKeys::Eight, ImGuiKey_8 },
			{ EKeys::NumPadEight, ImGuiKey_Keypad8 },
			{ EKeys::Nine, ImGuiKey_9 },
			{ EKeys::NumPadNine, ImGuiKey_Keypad9 },
			{ EKeys::F1, ImGuiKey_F1 },
			{ EKeys::F2, ImGuiKey_F2 },
			{ EKeys::F3, ImGuiKey_F3 },
			{ EKeys::F4, ImGuiKey_F4 },
			{ EKeys::F5, ImGuiKey_F5 },
			{ EKeys::F6, ImGuiKey_F6 },
			{ EKeys::F7, ImGuiKey_F7 },
			{ EKeys::F8, ImGuiKey_F8 },
			{ EKeys::F9, ImGuiKey_F9 },
			{ EKeys::F10, ImGuiKey_F10 },
			{ EKeys::F11, ImGuiKey_F11 },
			{ EKeys::F12, ImGuiKey_F12 },
		};
		const ImGuiKey* MappedKey = KeyMap.Find(Key);
		return MappedKey ? *MappedKey : ImGuiKey_None;
	}

	uint32 GetKeyIndex(const FKey& Key)
	{
		const ImGuiKey MappedKey = ToImGuiKey(Key);
		return MappedKey == ImGuiKey_None ? MAX_uint32 : static_cast<uint32>(MappedKey - ImGuiKey_NamedKey_BEGIN);
	}

	uint32 GetKeyIndex(const FKeyEvent& KeyEvent)
	{
		return GetKeyIndex(KeyEvent.GetKey());
	}

	uint32 GetMouseIndex(const FKey& MouseButton)
	{
		if (MouseButton == EKeys::LeftMouseButton) return 0;
		if (MouseButton == EKeys::RightMouseButton) return 1;
		if (MouseButton == EKeys::MiddleMouseButton) return 2;
		if (MouseButton == EKeys::ThumbMouseButton) return 3;
		if (MouseButton == EKeys::ThumbMouseButton2) return 4;
		return MAX_uint32;
	}

	EMouseCursor::Type ToSlateMouseCursor(ImGuiMouseCursor MouseCursor)
	{
		switch (MouseCursor)
		{
		case ImGuiMouseCursor_Arrow: return EMouseCursor::Default;
		case ImGuiMouseCursor_TextInput: return EMouseCursor::TextEditBeam;
		case ImGuiMouseCursor_ResizeAll: return EMouseCursor::CardinalCross;
		case ImGuiMouseCursor_ResizeNS: return EMouseCursor::ResizeUpDown;
		case ImGuiMouseCursor_ResizeEW: return EMouseCursor::ResizeLeftRight;
		case ImGuiMouseCursor_ResizeNESW: return EMouseCursor::ResizeSouthWest;
		case ImGuiMouseCursor_ResizeNWSE: return EMouseCursor::ResizeSouthEast;
		case ImGuiMouseCursor_Hand: return EMouseCursor::Hand;
		case ImGuiMouseCursor_NotAllowed: return EMouseCursor::SlashedCircle;
		default: return EMouseCursor::None;
		}
	}

	void SetGamepadNavigationKey(ImGuiTypes::FNavInputArray& NavInputs, const FKey& Key, bool bIsDown)
	{
		static const TMap<FKey, ImGuiKey> GamepadKeys = {
			{ EKeys::Gamepad_Special_Right, ImGuiKey_GamepadStart },
			{ EKeys::Gamepad_Special_Left, ImGuiKey_GamepadBack },
			{ EKeys::Gamepad_FaceButton_Bottom, ImGuiKey_GamepadFaceDown },
			{ EKeys::Gamepad_FaceButton_Right, ImGuiKey_GamepadFaceRight },
			{ EKeys::Gamepad_FaceButton_Top, ImGuiKey_GamepadFaceUp },
			{ EKeys::Gamepad_FaceButton_Left, ImGuiKey_GamepadFaceLeft },
			{ EKeys::Gamepad_DPad_Left, ImGuiKey_GamepadDpadLeft },
			{ EKeys::Gamepad_DPad_Right, ImGuiKey_GamepadDpadRight },
			{ EKeys::Gamepad_DPad_Up, ImGuiKey_GamepadDpadUp },
			{ EKeys::Gamepad_DPad_Down, ImGuiKey_GamepadDpadDown },
			{ EKeys::Gamepad_LeftShoulder, ImGuiKey_GamepadL1 },
			{ EKeys::Gamepad_RightShoulder, ImGuiKey_GamepadR1 },
			{ EKeys::Gamepad_LeftThumbstick, ImGuiKey_GamepadL3 },
			{ EKeys::Gamepad_RightThumbstick, ImGuiKey_GamepadR3 },
		};
		if (const ImGuiKey* MappedKey = GamepadKeys.Find(Key))
		{
			NavInputs[*MappedKey - ImGuiKey_GamepadStart] = bIsDown ? 1.f : 0.f;
		}
	}

	void SetGamepadNavigationAxis(ImGuiTypes::FNavInputArray& NavInputs, const FKey& Key, float Value)
	{
		const auto SetAxis = [&](ImGuiKey Negative, ImGuiKey Positive)
		{
			constexpr float DeadZone = 0.166f;
			NavInputs[Negative - ImGuiKey_GamepadStart] = FMath::Clamp((-Value - DeadZone) / (1.f - DeadZone), 0.f, 1.f);
			NavInputs[Positive - ImGuiKey_GamepadStart] = FMath::Clamp((Value - DeadZone) / (1.f - DeadZone), 0.f, 1.f);
		};
		if (Key == EKeys::Gamepad_LeftX) SetAxis(ImGuiKey_GamepadLStickLeft, ImGuiKey_GamepadLStickRight);
		else if (Key == EKeys::Gamepad_LeftY) SetAxis(ImGuiKey_GamepadLStickDown, ImGuiKey_GamepadLStickUp);
		else if (Key == EKeys::Gamepad_RightX) SetAxis(ImGuiKey_GamepadRStickLeft, ImGuiKey_GamepadRStickRight);
		else if (Key == EKeys::Gamepad_RightY) SetAxis(ImGuiKey_GamepadRStickDown, ImGuiKey_GamepadRStickUp);
		else if (Key == EKeys::Gamepad_LeftTriggerAxis) NavInputs[ImGuiKey_GamepadL2 - ImGuiKey_GamepadStart] = FMath::Clamp(Value, 0.f, 1.f);
		else if (Key == EKeys::Gamepad_RightTriggerAxis) NavInputs[ImGuiKey_GamepadR2 - ImGuiKey_GamepadStart] = FMath::Clamp(Value, 0.f, 1.f);
	}

	template<typename TFlags, typename TFlag>
	static void SetFlag(TFlags& Flags, TFlag Flag, bool bSet)
	{
		Flags = bSet ? Flags | Flag : Flags & ~Flag;
	}

	void CopyInput(ImGuiIO& IO, const FImGuiInputState& InputState)
	{
		ImGuiImplementation::ResetInput(IO, InputState.ShouldResetKeyboard(), InputState.ShouldResetMouse(), InputState.ShouldResetGamepad());
		const bool bGamepadEnabled = InputState.IsGamepadNavigationEnabled() && InputState.HasGamepad();
		SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard, InputState.IsKeyboardNavigationEnabled());
		SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad, InputState.IsGamepadNavigationEnabled());
		SetFlag(IO.BackendFlags, ImGuiBackendFlags_HasGamepad, InputState.HasGamepad());
		IO.MouseDrawCursor = InputState.HasMousePointer();

		// Preserve the order supplied by Slate, including mixed text/key and move/click sequences.
		for (const auto& Event : InputState.GetEvents())
		{
			switch (Event.Type)
			{
			case FImGuiInputState::EEventType::Key:
				IO.AddKeyEvent(Event.Key, Event.bIsDown);
				break;
			case FImGuiInputState::EEventType::Gamepad:
				if (bGamepadEnabled) IO.AddKeyAnalogEvent(Event.Key, Event.bIsDown, Event.X);
				break;
			case FImGuiInputState::EEventType::Character:
				if constexpr (sizeof(TCHAR) == 2) IO.AddInputCharacterUTF16(static_cast<ImWchar16>(Event.Character));
				else IO.AddInputCharacter(Event.Character);
				break;
			case FImGuiInputState::EEventType::MousePosition:
				if (InputState.IsTouchActive() && Event.MouseSource == ImGuiMouseSource_Mouse) break;
				IO.AddMouseSourceEvent(Event.MouseSource);
				IO.AddMousePosEvent(Event.X, Event.Y);
				break;
			case FImGuiInputState::EEventType::MouseButton:
				if (InputState.IsTouchActive() && Event.MouseSource == ImGuiMouseSource_Mouse && Event.Button == 0) break;
				IO.AddMouseSourceEvent(Event.MouseSource);
				IO.AddMouseButtonEvent(Event.Button, Event.bIsDown);
				break;
			case FImGuiInputState::EEventType::MouseWheel:
				if (!InputState.IsTouchActive()) IO.AddMouseWheelEvent(Event.X, Event.Y);
				break;
			case FImGuiInputState::EEventType::Focus:
				IO.AddFocusEvent(Event.bIsDown);
				break;
			}
		}
		if (!InputState.IsTouchActive())
		{
			// Restore the physical pointer after touch ends, including frames without mouse motion.
			// This follows queued events so it cannot move a click to a later position.
			IO.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
			const FVector2D& Position = InputState.GetMousePosition();
			IO.AddMousePosEvent(static_cast<float>(Position.X), static_cast<float>(Position.Y));
		}
	}
}
