// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <IConsoleManager.h>


// Collects and give access to module properties.
// TODO: For now singleton instance is initialized on the first use. Try to move it to the ImGui Manager. 
class FImGuiModuleProperties
{
public:

	// Get the instance of the ImGui properties.
	static FImGuiModuleProperties& Get();

	// Check whether ImGui input is enabled.
	bool IsInputEnabled() const { return bInputEnabled; }

	// Enable or disable ImGui input.
	void SetInputEnabled(bool bEnabled) { bInputEnabled = bEnabled; }

	// Toggle ImGui input.
	void ToggleInput() { SetInputEnabled(!IsInputEnabled()); }

	// Check whether keyboard navigation is enabled.
	bool IsKeyboardNavigationEnabled() const { return bKeyboardNavigationEnabled; }

	// Enable or disable keyboard navigation.
	void SetKeyboardNavigationEnabled(bool bEnabled) { bKeyboardNavigationEnabled = bEnabled; }

	// Toggle keyboard navigation.
	void ToggleKeyboardNavigation() { SetKeyboardNavigationEnabled(!IsKeyboardNavigationEnabled()); }

	// Check whether gamepad navigation is enabled.
	bool IsGamepadNavigationEnabled() const { return bGamepadNavigationEnabled; }

	// Enable or disable gamepad navigation.
	void SetGamepadNavigationEnabled(bool bEnabled) { bGamepadNavigationEnabled = bEnabled; }

	// Toggle gamepad navigation.
	void ToggleGamepadNavigation() { SetGamepadNavigationEnabled(!IsGamepadNavigationEnabled()); }

	// Check whether ImGui demo is visible.
	bool ShowDemo() const { return bShowDemo; }

	// Show or hide ImGui demo.
	void SetShowDemo(bool bShow) { bShowDemo = bShow; }

	// Toggle ImGui demo.
	void ToggleDemo() { SetShowDemo(!ShowDemo()); }

private:

	FImGuiModuleProperties() = default;

	FImGuiModuleProperties(const FImGuiModuleProperties&) = delete;
	FImGuiModuleProperties& operator=(const FImGuiModuleProperties&) = delete;

	FImGuiModuleProperties(FImGuiModuleProperties&&) = delete;
	FImGuiModuleProperties& operator=(FImGuiModuleProperties&&) = delete;

	bool bInputEnabled = false;

	bool bKeyboardNavigationEnabled = false;
	bool bGamepadNavigationEnabled = false;

	bool bShowDemo = false;
};
