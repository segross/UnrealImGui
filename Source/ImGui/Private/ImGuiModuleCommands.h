// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <IConsoleManager.h>


// Wrapper for ImGui console commands.
class FImGuiModuleCommands
{
	// Allow module manager to control life-cycle of this class.
	friend class FImGuiModuleManager;

	FImGuiModuleCommands();
	~FImGuiModuleCommands();

	FImGuiModuleCommands(const FImGuiModuleCommands&) = delete;
	FImGuiModuleCommands& operator=(const FImGuiModuleCommands&) = delete;

	FImGuiModuleCommands(FImGuiModuleCommands&&) = delete;
	FImGuiModuleCommands& operator=(FImGuiModuleCommands&&) = delete;

	void InitializeSettings();

	void RegisterSettingsDelegates();
	void UnregisterSettingsDelegates();

	void UpdateToggleInputKeyBinding();

	void ToggleInput();
	void ToggleKeyboardNavigation();
	void ToggleGamepadNavigation();
	void ToggleDemo();

	FAutoConsoleCommand ToggleInputCommand;
	FAutoConsoleCommand ToggleKeyboardNavigationCommand;
	FAutoConsoleCommand ToggleGamepadNavigationCommand;
	FAutoConsoleCommand ToggleDemoCommand;
};
