// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "DebugExecBindings.h"

#include "ImGuiModuleSettings.h"

#include <GameFramework/PlayerInput.h>
#include <UObject/UObjectIterator.h>


namespace
{
	FKeyBind CreateKeyBind(const FImGuiKeyInfo& KeyInfo, const FString& Command)
	{
		FKeyBind KeyBind;
		KeyBind.Command = Command;
		KeyBind.Key = KeyInfo.Key;
		KeyBind.bDisabled = false;

#define FILL_MODIFIER_DATA(KeyInfoProperty, BindProperty, BindIgnoreProperty)\
			if (KeyInfo.KeyInfoProperty == ECheckBoxState::Undetermined)\
			{\
				KeyBind.BindProperty = KeyBind.BindIgnoreProperty = false;\
			}\
			else\
			{\
				KeyBind.BindProperty = (KeyInfo.KeyInfoProperty == ECheckBoxState::Checked);\
				KeyBind.BindIgnoreProperty = !KeyBind.BindProperty;\
			}

		FILL_MODIFIER_DATA(Shift, Shift, bIgnoreShift);
		FILL_MODIFIER_DATA(Ctrl, Control, bIgnoreCtrl);
		FILL_MODIFIER_DATA(Alt, Alt, bIgnoreAlt);
		FILL_MODIFIER_DATA(Cmd, Cmd, bIgnoreCmd);

#undef FILL_MODIFIER_DATA

		return KeyBind;
	}

	bool IsBindable(const FKey& Key)
	{
#if ENGINE_COMPATIBILITY_LEGACY_KEY_AXIS_API
		return Key.IsValid() && Key != EKeys::AnyKey && !Key.IsFloatAxis() && !Key.IsVectorAxis()
			&& !Key.IsGamepadKey() && !Key.IsModifierKey() && !Key.IsMouseButton();
#else
		return Key.IsValid() && Key != EKeys::AnyKey && !Key.IsAxis1D() && !Key.IsAxis2D()
			&& !Key.IsAxis3D() && !Key.IsGamepadKey() && !Key.IsModifierKey() && !Key.IsMouseButton();
#endif
	}

	void UpdatePlayerInput(UPlayerInput* PlayerInput, const FKeyBind& KeyBind)
	{
		const int32 Index = PlayerInput->DebugExecBindings.IndexOfByPredicate([&](const FKeyBind& PlayerKeyBind)
		{
			return PlayerKeyBind.Command.Equals(KeyBind.Command, ESearchCase::IgnoreCase);
		});

		if (IsBindable(KeyBind.Key))
		{
			if (Index != INDEX_NONE)
			{
				PlayerInput->DebugExecBindings[Index] = KeyBind;
			}
			else
			{
				PlayerInput->DebugExecBindings.Add(KeyBind);
			}
		}
		else
		{
			if (Index != INDEX_NONE)
			{
				PlayerInput->DebugExecBindings.RemoveAt(Index);
			}
		}
	}
}

namespace DebugExecBindings
{
	void UpdatePlayerInputs(const FImGuiKeyInfo& KeyInfo, const FString& Command)
	{
		checkf(!Command.IsEmpty(), TEXT("Empty command."));

		const FKeyBind KeyBind = CreateKeyBind(KeyInfo, Command);

		// Update default player input, so changes will be visible in all PIE sessions created after this point.
		if (UPlayerInput* DefaultPlayerInput = GetMutableDefault<UPlayerInput>())
		{
			UpdatePlayerInput(DefaultPlayerInput, KeyBind);
		}

		// Update all existing player inputs to see changes in running PIE session.
		for (TObjectIterator<UPlayerInput> It; It; ++It)
		{
			UpdatePlayerInput(*It, KeyBind);
		}
	}
}
