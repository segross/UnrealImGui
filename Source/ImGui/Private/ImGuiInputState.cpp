// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiInputState.h"


FImGuiInputState::FImGuiInputState()
{
	ClearState();
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

void FImGuiInputState::ClearState()
{
	ClearCharacters();

	using std::fill;
	fill(KeysDown, &KeysDown[Utilities::GetArraySize(KeysDown)], false);
	fill(MouseButtonsDown, &MouseButtonsDown[Utilities::GetArraySize(MouseButtonsDown)], false);

	// Fully expanding dirty parts of both arrays, to inform about the change.
	KeysUpdateRange.SetFull();
	MouseButtonsUpdateRange.SetFull();
}

void FImGuiInputState::ClearUpdateState()
{
	ClearCharacters();

	KeysUpdateRange.SetEmpty();
	MouseButtonsUpdateRange.SetEmpty();

	MouseWheelDelta = 0.f;
}

void FImGuiInputState::ClearCharacters()
{
	InputCharactersNum = 0;
	InputCharacters[0];
}
