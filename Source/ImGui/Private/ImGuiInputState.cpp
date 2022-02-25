// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiInputState.h"

#include <algorithm>
#include <limits>
#include <type_traits>


FImGuiInputState::FImGuiInputState()
{
	Reset();
}

void FImGuiInputState::AddCharacter(TCHAR Char)
{
	InputCharacters.Add(Char);
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
	ClearCharacters();

	KeyDownEvents.Reset();
	KeyUpEvents.Reset();

	KeysUpdateRange.SetEmpty();
	MouseButtonsUpdateRange.SetEmpty();

	MouseWheelDelta = 0.f;

	bTouchProcessed = bTouchDown;
}

void FImGuiInputState::ClearCharacters()
{
	InputCharacters.Empty();
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

