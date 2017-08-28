// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiInputState.h"


FImGuiInputState::FImGuiInputState()
{
	ResetState();
}

void FImGuiInputState::AddCharacter(TCHAR Char)
{
	static_assert(sizeof(TCHAR) <= sizeof(InputCharacters[0]), "Size of elements in Input Characters buffer is smaller than size of 'TCHAR'. Possible truncation.");

	if (InputCharactersNum < Utilities::GetArraySize(InputCharacters))
	{
		InputCharacters[InputCharactersNum++] = static_cast<ImWchar>(Char);
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

void FImGuiInputState::Reset(bool bKeyboard, bool bMouse)
{
	if (bKeyboard)
	{
		ClearCharacters();
		ClearKeys();
	}

	if (bMouse)
	{
		ClearMouseButtons();
		ClearMouseAnalogue();
	}

	if (bKeyboard && bMouse)
	{
		ClearModifierKeys();
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

	// Expand update range because keys array has been updated.
	KeysUpdateRange.SetFull();
}

void FImGuiInputState::ClearMouseButtons()
{
	using std::fill;
	fill(MouseButtonsDown, &MouseButtonsDown[Utilities::GetArraySize(MouseButtonsDown)], false);

	// Expand update range because mouse buttons array has been updated.
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
