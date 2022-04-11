// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiInteroperability.h"

#include "ImGuiInputState.h"
#include "Utilities/Arrays.h"


// If TCHAR is wider than ImWchar, enable or disable validation of input character before conversions.
#define VALIDATE_INPUT_CHARACTERS 1

#if VALIDATE_INPUT_CHARACTERS
DEFINE_LOG_CATEGORY_STATIC(LogImGuiInput, Warning, All);
#endif

namespace
{
	//====================================================================================================
	// Character conversion
	//====================================================================================================

	template<typename T, std::enable_if_t<(sizeof(T) <= sizeof(ImWchar)), T>* = nullptr>
	ImWchar CastInputChar(T Char)
	{
		return static_cast<ImWchar>(Char);
	}

	template<typename T, std::enable_if_t<!(sizeof(T) <= sizeof(ImWchar)), T>* = nullptr>
	ImWchar CastInputChar(T Char)
	{
#if VALIDATE_INPUT_CHARACTERS
		// We only need a runtime validation if TCHAR is wider than ImWchar.
		// Signed and unsigned integral types with the same size as ImWchar should be safely converted. As long as the
		// char value is in that range we can safely use it, otherwise we should log an error to notify about possible
		// truncations.
		static constexpr auto MinLimit = (std::numeric_limits<std::make_signed_t<ImWchar>>::min)();
		static constexpr auto MaxLimit = (std::numeric_limits<std::make_unsigned_t<ImWchar>>::max)();
		UE_CLOG(!(Char >= MinLimit && Char <= MaxLimit), LogImGuiInput, Error,
			TEXT("TCHAR value '%c' (%#x) is out of range %d (%#x) to %u (%#x) that can be safely converted to ImWchar. ")
			TEXT("If you wish to disable this validation, please set VALIDATE_INPUT_CHARACTERS in ImGuiInputState.cpp to 0."),
			Char, Char, MinLimit, MinLimit, MaxLimit, MaxLimit);
#endif

		return static_cast<ImWchar>(Char);
	}

	//====================================================================================================
	// Copying Utilities
	//====================================================================================================

	// Copy all elements from source to destination array of the same size.
	template<typename TArray>
	void Copy(const TArray& Src, TArray& Dst)
	{
		using std::copy;
		using std::begin;
		using std::end;
		copy(begin(Src), end(Src), begin(Dst));
	}

	// Copy subrange of source array to destination array of the same size.
	template<typename TArray, typename SizeType>
	void Copy(const TArray& Src, TArray& Dst, const Utilities::TArrayIndexRange<TArray, SizeType>& Range)
	{
		using std::copy;
		using std::begin;
		copy(begin(Src) + Range.GetBegin(), begin(Src) + Range.GetEnd(), begin(Dst) + Range.GetBegin());
	}

	// Copy number of elements from the beginning of source array to the beginning of destination array of the same size.
	template<typename TArray, typename SizeType>
	void Copy(const TArray& Src, TArray& Dst, SizeType Count)
	{
		checkf(Count < Utilities::ArraySize<TArray>::value, TEXT("Number of copied elements is larger than array size."));

		using std::copy;
		using std::begin;
		copy(begin(Src), begin(Src) + Count, begin(Dst));
	}
}

namespace ImGuiInterops
{
	//====================================================================================================
	// Input Mapping
	//====================================================================================================

	static TMap<FKey, ImGuiKey> UnrealToImGuiKeyMap;

	void SetUnrealKeyMap(ImGuiIO& IO)
	{
		UnrealToImGuiKeyMap.Add(EKeys::Tab, ImGuiKey_Tab);
		
		UnrealToImGuiKeyMap.Add(EKeys::Left,  ImGuiKey_LeftArrow);
		UnrealToImGuiKeyMap.Add(EKeys::Right, ImGuiKey_RightArrow);
		UnrealToImGuiKeyMap.Add(EKeys::Up,    ImGuiKey_UpArrow);
		UnrealToImGuiKeyMap.Add(EKeys::Down,  ImGuiKey_DownArrow);
		
		UnrealToImGuiKeyMap.Add(EKeys::PageUp,   ImGuiKey_PageUp);
		UnrealToImGuiKeyMap.Add(EKeys::PageDown, ImGuiKey_PageDown);
		UnrealToImGuiKeyMap.Add(EKeys::Home,     ImGuiKey_Home);
		UnrealToImGuiKeyMap.Add(EKeys::End,      ImGuiKey_End);
		UnrealToImGuiKeyMap.Add(EKeys::Insert,   ImGuiKey_Insert);
		UnrealToImGuiKeyMap.Add(EKeys::Delete,   ImGuiKey_Delete);

		UnrealToImGuiKeyMap.Add(EKeys::NumLock,    ImGuiKey_NumLock);
		UnrealToImGuiKeyMap.Add(EKeys::ScrollLock, ImGuiKey_ScrollLock);
		UnrealToImGuiKeyMap.Add(EKeys::Pause,      ImGuiKey_Pause);
		
		UnrealToImGuiKeyMap.Add(EKeys::BackSpace, ImGuiKey_Backspace);
		UnrealToImGuiKeyMap.Add(EKeys::SpaceBar,  ImGuiKey_Space);
		UnrealToImGuiKeyMap.Add(EKeys::Enter,     ImGuiKey_Enter);
		UnrealToImGuiKeyMap.Add(EKeys::Escape,     ImGuiKey_Escape);
		
		UnrealToImGuiKeyMap.Add(EKeys::A, ImGuiKey_A);
		UnrealToImGuiKeyMap.Add(EKeys::B, ImGuiKey_B);
		UnrealToImGuiKeyMap.Add(EKeys::C, ImGuiKey_C);
		UnrealToImGuiKeyMap.Add(EKeys::D, ImGuiKey_D);
		UnrealToImGuiKeyMap.Add(EKeys::E, ImGuiKey_E);
		UnrealToImGuiKeyMap.Add(EKeys::F, ImGuiKey_F);
		UnrealToImGuiKeyMap.Add(EKeys::G, ImGuiKey_G);
		UnrealToImGuiKeyMap.Add(EKeys::H, ImGuiKey_H);
		UnrealToImGuiKeyMap.Add(EKeys::I, ImGuiKey_I);
		UnrealToImGuiKeyMap.Add(EKeys::J, ImGuiKey_J);
		UnrealToImGuiKeyMap.Add(EKeys::K, ImGuiKey_K);
		UnrealToImGuiKeyMap.Add(EKeys::L, ImGuiKey_L);
		UnrealToImGuiKeyMap.Add(EKeys::M, ImGuiKey_M);
		UnrealToImGuiKeyMap.Add(EKeys::N, ImGuiKey_N);
		UnrealToImGuiKeyMap.Add(EKeys::O, ImGuiKey_O);
		UnrealToImGuiKeyMap.Add(EKeys::P, ImGuiKey_P);
		UnrealToImGuiKeyMap.Add(EKeys::Q, ImGuiKey_Q);
		UnrealToImGuiKeyMap.Add(EKeys::R, ImGuiKey_R);
		UnrealToImGuiKeyMap.Add(EKeys::S, ImGuiKey_S);
		UnrealToImGuiKeyMap.Add(EKeys::T, ImGuiKey_T);
		UnrealToImGuiKeyMap.Add(EKeys::U, ImGuiKey_U);
		UnrealToImGuiKeyMap.Add(EKeys::V, ImGuiKey_V);
		UnrealToImGuiKeyMap.Add(EKeys::W, ImGuiKey_W);
		UnrealToImGuiKeyMap.Add(EKeys::X, ImGuiKey_X);
		UnrealToImGuiKeyMap.Add(EKeys::Y, ImGuiKey_Y);
		UnrealToImGuiKeyMap.Add(EKeys::Z, ImGuiKey_Z);
		
		UnrealToImGuiKeyMap.Add(EKeys::F1,  ImGuiKey_F1);
		UnrealToImGuiKeyMap.Add(EKeys::F2,  ImGuiKey_F2);
		UnrealToImGuiKeyMap.Add(EKeys::F3,  ImGuiKey_F3);
		UnrealToImGuiKeyMap.Add(EKeys::F4,  ImGuiKey_F4);
		UnrealToImGuiKeyMap.Add(EKeys::F5,  ImGuiKey_F5);
		UnrealToImGuiKeyMap.Add(EKeys::F6,  ImGuiKey_F6);
		UnrealToImGuiKeyMap.Add(EKeys::F7,  ImGuiKey_F7);
		UnrealToImGuiKeyMap.Add(EKeys::F8,  ImGuiKey_F8);
		UnrealToImGuiKeyMap.Add(EKeys::F9,  ImGuiKey_F9);
		UnrealToImGuiKeyMap.Add(EKeys::F10, ImGuiKey_F10);
		UnrealToImGuiKeyMap.Add(EKeys::F11, ImGuiKey_F11);
		UnrealToImGuiKeyMap.Add(EKeys::F12, ImGuiKey_F12);
		
		UnrealToImGuiKeyMap.Add(EKeys::One,   ImGuiKey_0);
		UnrealToImGuiKeyMap.Add(EKeys::Two,   ImGuiKey_1);
		UnrealToImGuiKeyMap.Add(EKeys::Three, ImGuiKey_2);
		UnrealToImGuiKeyMap.Add(EKeys::Four,  ImGuiKey_3);
		UnrealToImGuiKeyMap.Add(EKeys::Five,  ImGuiKey_4);
		UnrealToImGuiKeyMap.Add(EKeys::Six,   ImGuiKey_5);
		UnrealToImGuiKeyMap.Add(EKeys::Seven, ImGuiKey_6);
		UnrealToImGuiKeyMap.Add(EKeys::Eight, ImGuiKey_7);
		UnrealToImGuiKeyMap.Add(EKeys::Nine,  ImGuiKey_8);
		
		UnrealToImGuiKeyMap.Add(EKeys::Equals,       ImGuiKey_Equal);
		UnrealToImGuiKeyMap.Add(EKeys::Comma,        ImGuiKey_Comma);
		UnrealToImGuiKeyMap.Add(EKeys::Period,       ImGuiKey_Period);
		UnrealToImGuiKeyMap.Add(EKeys::Slash,        ImGuiKey_Slash);
		UnrealToImGuiKeyMap.Add(EKeys::LeftBracket,  ImGuiKey_LeftBracket);
		UnrealToImGuiKeyMap.Add(EKeys::RightBracket, ImGuiKey_RightBracket);
		UnrealToImGuiKeyMap.Add(EKeys::Apostrophe,   ImGuiKey_Apostrophe);
		UnrealToImGuiKeyMap.Add(EKeys::Semicolon,    ImGuiKey_Semicolon);

		UnrealToImGuiKeyMap.Add(EKeys::NumPadZero,  ImGuiKey_Keypad0);
		UnrealToImGuiKeyMap.Add(EKeys::NumPadOne,   ImGuiKey_Keypad1);
		UnrealToImGuiKeyMap.Add(EKeys::NumPadTwo,   ImGuiKey_Keypad2);
		UnrealToImGuiKeyMap.Add(EKeys::NumPadThree, ImGuiKey_Keypad3);
		UnrealToImGuiKeyMap.Add(EKeys::NumPadFour,  ImGuiKey_Keypad4);
		UnrealToImGuiKeyMap.Add(EKeys::NumPadFive,  ImGuiKey_Keypad5);
		UnrealToImGuiKeyMap.Add(EKeys::NumPadSix,   ImGuiKey_Keypad6);
		UnrealToImGuiKeyMap.Add(EKeys::NumPadSeven, ImGuiKey_Keypad7);
		UnrealToImGuiKeyMap.Add(EKeys::NumPadEight, ImGuiKey_Keypad8);
		UnrealToImGuiKeyMap.Add(EKeys::NumPadNine,  ImGuiKey_Keypad9);

		UnrealToImGuiKeyMap.Add(EKeys::Multiply, ImGuiKey_KeypadMultiply);
		UnrealToImGuiKeyMap.Add(EKeys::Add,      ImGuiKey_KeypadAdd);
		UnrealToImGuiKeyMap.Add(EKeys::Subtract, ImGuiKey_KeypadSubtract);
		UnrealToImGuiKeyMap.Add(EKeys::Decimal,  ImGuiKey_KeypadDecimal);
		UnrealToImGuiKeyMap.Add(EKeys::Divide,   ImGuiKey_KeypadDivide);
	}

	// Simple transform mapping key codes to 0-511 range used in ImGui.
	// From what I can tell, on most supported platforms key codes should comfortably fit in that range anyway
	// but the SDL key-codes used on Linux can go way out of this range (because of the extra flag). However,
	// after this transform they should fit in the range without conflicts.
	// NOTE: Should any of the platforms have other conflicts or any trouble with inputs, this is the likely
	// candidate for change.
	static uint32 MapKeyCode(uint32 KeyCode)
	{
		return (KeyCode < 512) ? KeyCode : 256 + (KeyCode % 256);
	}

	uint32 GetKeyIndex(const FKey& Key)
	{
		const uint32* pKeyCode = nullptr;
		const uint32* pCharCode = nullptr;

		FInputKeyManager::Get().GetCodesFromKey(Key, pKeyCode, pCharCode);

		const uint32 KeyCode =
			pKeyCode ? *pKeyCode
			: pCharCode ? *pCharCode
			: 0;

		return MapKeyCode(KeyCode);
	}

	uint32 GetKeyIndex(const FKeyEvent& KeyEvent)
	{
		return MapKeyCode(KeyEvent.GetKeyCode());
	}

	uint32 GetMouseIndex(const FKey& MouseButton)
	{
		if (MouseButton == EKeys::LeftMouseButton)
		{
			return 0;
		}
		else if (MouseButton == EKeys::MiddleMouseButton)
		{
			return 2;
		}
		else if (MouseButton == EKeys::RightMouseButton)
		{
			return 1;
		}
		else if (MouseButton == EKeys::ThumbMouseButton)
		{
			return 3;
		}
		else if (MouseButton == EKeys::ThumbMouseButton2)
		{
			return 4;
		}

		return -1;
	}

	EMouseCursor::Type ToSlateMouseCursor(ImGuiMouseCursor MouseCursor)
	{
		switch (MouseCursor)
		{
		case ImGuiMouseCursor_Arrow:
			return EMouseCursor::Default;
		case ImGuiMouseCursor_TextInput:
			return EMouseCursor::TextEditBeam;
		case ImGuiMouseCursor_ResizeAll:
			return EMouseCursor::CardinalCross;
		case ImGuiMouseCursor_ResizeNS:
			return  EMouseCursor::ResizeUpDown;
		case ImGuiMouseCursor_ResizeEW:
			return  EMouseCursor::ResizeLeftRight;
		case ImGuiMouseCursor_ResizeNESW:
			return  EMouseCursor::ResizeSouthWest;
		case ImGuiMouseCursor_ResizeNWSE:
			return  EMouseCursor::ResizeSouthEast;
		case ImGuiMouseCursor_None:
		default:
			return EMouseCursor::None;
		}
	}

	namespace
	{
		inline void UpdateKey(const FKey& Key, const FKey& KeyCondition, float& Value, bool bIsDown)
		{
			if (Key == KeyCondition)
			{
				Value = (bIsDown) ? 1.f : 0.f;
			}
		}

		inline void UpdateAxisValues(float& Axis, float& Opposite, float Value)
		{
			constexpr float AxisInputThreshold = 0.166f;

			// Filter out small values to avoid false positives (helpful in case of worn controllers).
			Axis = FMath::Max(0.f, Value - AxisInputThreshold);
			Opposite = 0.f;
		}

		inline void UpdateSymmetricAxis(const FKey& Key, const FKey& KeyCondition, float& Negative, float& Positive, float Value)
		{
			if (Key == KeyCondition)
			{
				if (Value < 0.f)
				{
					UpdateAxisValues(Negative, Positive, -Value);
				}
				else
				{
					UpdateAxisValues(Positive, Negative, Value);
				}
			}
		}
	}

	void SetGamepadNavigationKey(ImGuiTypes::FNavInputArray& NavInputs, const FKey& Key, bool bIsDown)
	{
#define MAP_KEY(KeyCondition, NavIndex) UpdateKey(Key, KeyCondition, NavInputs[NavIndex], bIsDown)

		if (Key.IsGamepadKey())
		{
			MAP_KEY(EKeys::Gamepad_FaceButton_Bottom, ImGuiNavInput_Activate);
			MAP_KEY(EKeys::Gamepad_FaceButton_Right, ImGuiNavInput_Cancel);
			MAP_KEY(EKeys::Gamepad_FaceButton_Top, ImGuiNavInput_Input);
			MAP_KEY(EKeys::Gamepad_FaceButton_Left, ImGuiNavInput_Menu);
			MAP_KEY(EKeys::Gamepad_DPad_Left, ImGuiNavInput_DpadLeft);
			MAP_KEY(EKeys::Gamepad_DPad_Right, ImGuiNavInput_DpadRight);
			MAP_KEY(EKeys::Gamepad_DPad_Up, ImGuiNavInput_DpadUp);
			MAP_KEY(EKeys::Gamepad_DPad_Down, ImGuiNavInput_DpadDown);
			MAP_KEY(EKeys::Gamepad_LeftShoulder, ImGuiNavInput_FocusPrev);
			MAP_KEY(EKeys::Gamepad_RightShoulder, ImGuiNavInput_FocusNext);
			MAP_KEY(EKeys::Gamepad_LeftShoulder, ImGuiNavInput_TweakSlow);
			MAP_KEY(EKeys::Gamepad_RightShoulder, ImGuiNavInput_TweakFast);
		}

#undef MAP_KEY
	}

	void SetGamepadNavigationAxis(ImGuiTypes::FNavInputArray& NavInputs, const FKey& Key, float Value)
	{
#define MAP_SYMMETRIC_AXIS(KeyCondition, NegNavIndex, PosNavIndex) UpdateSymmetricAxis(Key, KeyCondition, NavInputs[NegNavIndex], NavInputs[PosNavIndex], Value)

		if (Key.IsGamepadKey())
		{
			MAP_SYMMETRIC_AXIS(EKeys::Gamepad_LeftX, ImGuiNavInput_LStickLeft, ImGuiNavInput_LStickRight);
			MAP_SYMMETRIC_AXIS(EKeys::Gamepad_LeftY, ImGuiNavInput_LStickDown, ImGuiNavInput_LStickUp);
		}

#undef MAP_SYMMETRIC_AXIS
	}

	//====================================================================================================
	// Input State Copying
	//====================================================================================================

	template<typename TFlags, typename TFlag>
	static inline constexpr void SetFlag(TFlags& Flags, TFlag Flag, bool bSet)
	{
		Flags = bSet ? Flags | Flag : Flags & ~Flag;
	}

	void CopyInput(ImGuiIO& IO, const FImGuiInputState& InputState)
	{
		static const uint32 LeftControl = GetKeyIndex(EKeys::LeftControl);
		static const uint32 RightControl = GetKeyIndex(EKeys::RightControl);
		static const uint32 LeftShift = GetKeyIndex(EKeys::LeftShift);
		static const uint32 RightShift = GetKeyIndex(EKeys::RightShift);
		static const uint32 LeftAlt = GetKeyIndex(EKeys::LeftAlt);
		static const uint32 RightAlt = GetKeyIndex(EKeys::RightAlt);

		// Copy key modifiers.
		IO.KeyCtrl = InputState.IsControlDown();
		IO.KeyShift = InputState.IsShiftDown();
		IO.KeyAlt = InputState.IsAltDown();
		IO.KeySuper = false;

		// Copy buffers.
		if (!InputState.GetKeysUpdateRange().IsEmpty())
		{
			// Key down events
			for(const auto& Pair : InputState.KeyDownEvents)
			{
				if(UnrealToImGuiKeyMap.Contains(Pair.Value.GetKey()))
				{
					IO.AddKeyEvent(UnrealToImGuiKeyMap[Pair.Value.GetKey()], true);
				}
			}

			// Key up events
			for(const auto& Pair : InputState.KeyUpEvents)
			{
				if(UnrealToImGuiKeyMap.Contains(Pair.Value.GetKey()))
				{
					IO.AddKeyEvent(UnrealToImGuiKeyMap[Pair.Value.GetKey()], false);
				}
			}
		}

		if (!InputState.GetMouseButtonsUpdateRange().IsEmpty())
		{
			Copy(InputState.GetMouseButtons(), IO.MouseDown, InputState.GetMouseButtonsUpdateRange());
		}

		for (const TCHAR Char : InputState.GetCharacters())
		{
			IO.AddInputCharacter(CastInputChar(Char));
		}

		if (InputState.IsGamepadNavigationEnabled() && InputState.HasGamepad())
		{
			Copy(InputState.GetNavigationInputs(), IO.NavInputs);
		}

		SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard, InputState.IsKeyboardNavigationEnabled());
		SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad, InputState.IsGamepadNavigationEnabled());
		SetFlag(IO.BackendFlags, ImGuiBackendFlags_HasGamepad, InputState.HasGamepad());

		// Check whether we need to draw cursor.
		IO.MouseDrawCursor = InputState.HasMousePointer();

		// If touch is enabled and active, give it a precedence.
		if (InputState.IsTouchActive())
		{
			// Copy the touch position to mouse position.
			IO.MousePos.x = InputState.GetTouchPosition().X;
			IO.MousePos.y = InputState.GetTouchPosition().Y;

			// With touch active one frame longer than it is down, we have one frame to processed touch up.
			IO.MouseDown[0] = InputState.IsTouchDown();
		}
		else
		{
			// Copy the mouse position.
			IO.MousePos.x = InputState.GetMousePosition().X;
			IO.MousePos.y = InputState.GetMousePosition().Y;

			// Copy mouse wheel delta.
			IO.MouseWheel += InputState.GetMouseWheelDelta();
		}
	}
}
