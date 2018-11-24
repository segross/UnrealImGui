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

	// Disable copy semantics.
	FImGuiModuleCommands(const FImGuiModuleCommands&) = default;
	FImGuiModuleCommands& operator=(const FImGuiModuleCommands&) = default;

	// Disable move semantics.
	FImGuiModuleCommands(FImGuiModuleCommands&&) = default;
	FImGuiModuleCommands& operator=(FImGuiModuleCommands&&) = default;

	void InitializeSettings();

	void RegisterSettingsDelegates();
	void UnregisterSettingsDelegates();

	void UpdateToggleInputKeyBinding();

	void ToggleInput();

	FAutoConsoleCommand ToggleInputCommand;
};
