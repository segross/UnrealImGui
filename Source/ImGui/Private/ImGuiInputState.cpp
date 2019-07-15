// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiInputState.h"

#include <algorithm>
#include <limits>
#include <type_traits>


// If TCHAR is wider than ImWchar, enable or disable validation of input character before conversions.
#define VALIDATE_INPUT_CHARACTERS 1

#if VALIDATE_INPUT_CHARACTERS
DEFINE_LOG_CATEGORY_STATIC(LogImGuiInput, Warning, All);
#endif

namespace
{
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
}

FImGuiInputState::FImGuiInputState()
{
	Reset();
}

void FImGuiInputState::AddCharacter(TCHAR Char)
{
	if (InputCharactersNum < Utilities::GetArraySize(InputCharacters))
	{
		InputCharacters[InputCharactersNum++] = CastInputChar(Char);
		InputCharacters[InputCharactersNum] = 0;
	}
}

void FImGuiInputState::SetKeyDown(uint32 KeyIndex, bool bIsDown)
{
	if (KeyIndex < Utilities::GetArraySize(KeysDown))
	{
		if (KeysDown[KeyIndex] != bIsDown)
		{
			KeysDown[KeyIndex] = bIsDown;
			KeysUpdateRange.AddPosition(KeyIndex);
		}
	}
}

void FImGuiInputState::SetMouseDown(uint32 MouseIndex, bool bIsDown)
{
	if (MouseIndex < Utilities::GetArraySize(MouseButtonsDown))
	{
		if (MouseButtonsDown[MouseIndex] != bIsDown)
		{
			MouseButtonsDown[MouseIndex] = bIsDown;
			MouseButtonsUpdateRange.AddPosition(MouseIndex);
		}
	}
}

void FImGuiInputState::ClearUpdateState()
{
	if (InputCharactersNum > 0)
	{
		ClearCharacters();
	}

	KeysUpdateRange.SetEmpty();
	MouseButtonsUpdateRange.SetEmpty();

	MouseWheelDelta = 0.f;
}

void FImGuiInputState::ClearCharacters()
{
	using std::fill;
	fill(InputCharacters, &InputCharacters[Utilities::GetArraySize(InputCharacters)], 0);
	InputCharactersNum = 0;
}

void FImGuiInputState::ClearKeys()
{
	using std::fill;
	fill(KeysDown, &KeysDown[Utilities::GetArraySize(KeysDown)], false);

	// Mark the whole array as dirty because potentially each entry could be affected.
	KeysUpdateRange.SetFull();
}

void FImGuiInputState::ClearMouseButtons()
{
	using std::fill;
	fill(MouseButtonsDown, &MouseButtonsDown[Utilities::GetArraySize(MouseButtonsDown)], false);

	// Mark the whole array as dirty because potentially each entry could be affected.
	MouseButtonsUpdateRange.SetFull();
}

void FImGuiInputState::ClearMouseAnalogue()
{
	MousePosition = FVector2D::ZeroVector;
	MouseWheelDelta = 0.f;
}

void FImGuiInputState::ClearModifierKeys()
{
	bIsControlDown = false;
	bIsShiftDown = false;
	bIsAltDown = false;
}

void FImGuiInputState::ClearNavigationInputs()
{
	using std::fill;
	fill(NavigationInputs, &NavigationInputs[Utilities::GetArraySize(NavigationInputs)], 0.f);
}

