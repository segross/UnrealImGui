// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiInteroperability.h"
#include "ImGuiInputState.h"
#include "Utilities/Arrays.h"


namespace
{
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

	void SetUnrealKeyMap(ImGuiIO& IO)
	{
		struct FUnrealToImGuiMapping
		{
			FUnrealToImGuiMapping()
			{
				KeyMap[ImGuiKey_Tab] = GetKeyIndex(EKeys::Tab);
				KeyMap[ImGuiKey_LeftArrow] = GetKeyIndex(EKeys::Left);
				KeyMap[ImGuiKey_RightArrow] = GetKeyIndex(EKeys::Right);
				KeyMap[ImGuiKey_UpArrow] = GetKeyIndex(EKeys::Up);
				KeyMap[ImGuiKey_DownArrow] = GetKeyIndex(EKeys::Down);
				KeyMap[ImGuiKey_PageUp] = GetKeyIndex(EKeys::PageUp);
				KeyMap[ImGuiKey_PageDown] = GetKeyIndex(EKeys::PageDown);
				KeyMap[ImGuiKey_Home] = GetKeyIndex(EKeys::Home);
				KeyMap[ImGuiKey_End] = GetKeyIndex(EKeys::End);
				KeyMap[ImGuiKey_Insert] = GetKeyIndex(EKeys::Insert);
				KeyMap[ImGuiKey_Delete] = GetKeyIndex(EKeys::Delete);
				KeyMap[ImGuiKey_Backspace] = GetKeyIndex(EKeys::BackSpace);
				KeyMap[ImGuiKey_Space] = GetKeyIndex(EKeys::SpaceBar);
				KeyMap[ImGuiKey_Enter] = GetKeyIndex(EKeys::Enter);
				KeyMap[ImGuiKey_Escape] = GetKeyIndex(EKeys::Escape);
				KeyMap[ImGuiKey_A] = GetKeyIndex(EKeys::A);
				KeyMap[ImGuiKey_C] = GetKeyIndex(EKeys::C);
				KeyMap[ImGuiKey_V] = GetKeyIndex(EKeys::V);
				KeyMap[ImGuiKey_X] = GetKeyIndex(EKeys::X);
				KeyMap[ImGuiKey_Y] = GetKeyIndex(EKeys::Y);
				KeyMap[ImGuiKey_Z] = GetKeyIndex(EKeys::Z);
			}

			ImGuiTypes::FKeyMap KeyMap;
		};

		static const FUnrealToImGuiMapping Mapping;

		Copy(Mapping.KeyMap, IO.KeyMap);
	}

	uint32 GetKeyIndex(const FKey& Key)
	{
		const uint32* pKeyCode = nullptr;
		const uint32* pCharCode = nullptr;

		FInputKeyManager::Get().GetCodesFromKey(Key, pKeyCode, pCharCode);

		if (pKeyCode)
		{
			return *pKeyCode;
		}

		if (pCharCode)
		{
			return *pCharCode;
		}

		return 0;
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

		// Check whether we need to draw cursor.
		IO.MouseDrawCursor = InputState.HasMousePointer();

		// Copy mouse position.
		IO.MousePos.x = InputState.GetMousePosition().X;
		IO.MousePos.y = InputState.GetMousePosition().Y;

		// Copy mouse wheel delta.
		IO.MouseWheel += InputState.GetMouseWheelDelta();

		// Copy key modifiers.
		IO.KeyCtrl = InputState.IsControlDown();
		IO.KeyShift = InputState.IsShiftDown();
		IO.KeyAlt = InputState.IsAltDown();
		IO.KeySuper = false;

		// Copy buffers.
		if (!InputState.GetKeysUpdateRange().IsEmpty())
		{
			Copy(InputState.GetKeys(), IO.KeysDown, InputState.GetKeysUpdateRange());
		}

		if (!InputState.GetMouseButtonsUpdateRange().IsEmpty())
		{
			Copy(InputState.GetMouseButtons(), IO.MouseDown, InputState.GetMouseButtonsUpdateRange());
		}

		if (InputState.GetCharactersNum() > 0)
		{
			Copy(InputState.GetCharacters(), IO.InputCharacters);
		}

		if (InputState.IsGamepadNavigationEnabled() && InputState.HasGamepad())
		{
			Copy(InputState.GetNavigationInputs(), IO.NavInputs);
		}

		SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard, InputState.IsKeyboardNavigationEnabled());
		SetFlag(IO.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad, InputState.IsGamepadNavigationEnabled());
		SetFlag(IO.BackendFlags, ImGuiBackendFlags_HasGamepad, InputState.HasGamepad());
	}
}
