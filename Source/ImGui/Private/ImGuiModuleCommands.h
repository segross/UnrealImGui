// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <IConsoleManager.h>


class FImGuiModuleManager;

// Manges ImGui module console commands.
class FImGuiModuleCommands
{
public:

	FImGuiModuleCommands(FImGuiModuleManager& InModuleManager);
	~FImGuiModuleCommands();

private:

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

	FImGuiModuleManager& ModuleManager;

	FAutoConsoleCommand ToggleInputCommand;
	FAutoConsoleCommand ToggleKeyboardNavigationCommand;
	FAutoConsoleCommand ToggleGamepadNavigationCommand;
	FAutoConsoleCommand ToggleDemoCommand;
};
