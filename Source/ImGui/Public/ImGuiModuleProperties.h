// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once


/** Properties that define state of the ImGui module. */
class IMGUI_API FImGuiModuleProperties
{
public:

	/** Check whether input is enabled. */
	bool IsInputEnabled() const { return bInputEnabled; }

	/** Enable or disable ImGui input. */
	void SetInputEnabled(bool bEnabled) { bInputEnabled = bEnabled; }

	/** Toggle ImGui input. */
	void ToggleInput() { SetInputEnabled(!IsInputEnabled()); }

	/** Check whether keyboard navigation is enabled. */
	bool IsKeyboardNavigationEnabled() const { return bKeyboardNavigationEnabled; }

	/** Enable or disable keyboard navigation. */
	void SetKeyboardNavigationEnabled(bool bEnabled) { bKeyboardNavigationEnabled = bEnabled; }

	/** Toggle keyboard navigation. */
	void ToggleKeyboardNavigation() { SetKeyboardNavigationEnabled(!IsKeyboardNavigationEnabled()); }

	/** Check whether gamepad navigation is enabled. */
	bool IsGamepadNavigationEnabled() const { return bGamepadNavigationEnabled; }

	/** Enable or disable gamepad navigation. */
	void SetGamepadNavigationEnabled(bool bEnabled) { bGamepadNavigationEnabled = bEnabled; }

	/** Toggle gamepad navigation. */
	void ToggleGamepadNavigation() { SetGamepadNavigationEnabled(!IsGamepadNavigationEnabled()); }

	/** Check whether ImGui demo is visible. */
	bool ShowDemo() const { return bShowDemo; }

	/** Show or hide ImGui demo. */
	void SetShowDemo(bool bShow) { bShowDemo = bShow; }

	/** Toggle ImGui demo. */
	void ToggleDemo() { SetShowDemo(!ShowDemo()); }

private:

	bool bInputEnabled = false;

	bool bKeyboardNavigationEnabled = false;
	bool bGamepadNavigationEnabled = false;

	bool bShowDemo = false;
};
