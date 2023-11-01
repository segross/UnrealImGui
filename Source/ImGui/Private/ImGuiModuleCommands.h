// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <HAL/IConsoleManager.h>


struct FImGuiKeyInfo;
class FImGuiModuleProperties;

// Manges ImGui module console commands.
class FImGuiModuleCommands
{
public:

	static const TCHAR* const ToggleInput;
	static const TCHAR* const ToggleKeyboardNavigation;
	static const TCHAR* const ToggleGamepadNavigation;
	static const TCHAR* const ToggleKeyboardInputSharing;
	static const TCHAR* const ToggleGamepadInputSharing;
	static const TCHAR* const ToggleMouseInputSharing;
	static const TCHAR* const SetMouseInputSharing;
	static const TCHAR* const ToggleDemo;

	FImGuiModuleCommands(FImGuiModuleProperties& InProperties);

	void SetKeyBinding(const TCHAR* CommandName, const FImGuiKeyInfo& KeyInfo);

private:

	void ToggleInputImpl();
	void ToggleKeyboardNavigationImpl();
	void ToggleGamepadNavigationImpl();
	void ToggleKeyboardInputSharingImpl();
	void ToggleGamepadInputSharingImpl();
	void ToggleMouseInputSharingImpl();
	void SetMouseInputSharingImpl(const TArray< FString >& Args);
	void ToggleDemoImpl();

	FImGuiModuleProperties& Properties;

	FAutoConsoleCommand ToggleInputCommand;
	FAutoConsoleCommand ToggleKeyboardNavigationCommand;
	FAutoConsoleCommand ToggleGamepadNavigationCommand;
	FAutoConsoleCommand ToggleKeyboardInputSharingCommand;
	FAutoConsoleCommand ToggleGamepadInputSharingCommand;
	FAutoConsoleCommand ToggleMouseInputSharingCommand;
	FAutoConsoleCommand SetMouseInputSharingCommand;
	FAutoConsoleCommand ToggleDemoCommand;
};
