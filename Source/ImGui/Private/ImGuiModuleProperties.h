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

	// Check whether input is enabled.
	bool IsInputEnabled() const;

	// Set whether input should be enabled.
	void SetInputEnabled(bool bEnabled, EConsoleVariableFlags SetBy = ECVF_SetByCode);

	// Toggle input state.
	void ToggleInput(EConsoleVariableFlags SetBy = ECVF_SetByCode);

	// Check whether keyboard navigation is enabled.
	bool IsKeyboardNavigationEnabled() const;

	// Check whether gamepad navigation is enabled.
	bool IsGamepadNavigationEnabled() const;

	// Check whether demo should be visible.
	bool ShowDemo() const;

	// Set whether demo should be visible.
	void SetShowDemo(bool bEnabled, EConsoleVariableFlags SetBy = ECVF_SetByCode);

	// Toggle demo visibility.
	void ToggleDemo(EConsoleVariableFlags SetBy = ECVF_SetByCode);

private:

	FImGuiModuleProperties();

	// Disable copy and move semantics.
	FImGuiModuleProperties(const FImGuiModuleProperties&) = delete;
	FImGuiModuleProperties& operator=(const FImGuiModuleProperties&) = delete;

	FImGuiModuleProperties(FImGuiModuleProperties&&) = delete;
	FImGuiModuleProperties& operator=(FImGuiModuleProperties&&) = delete;

	TAutoConsoleVariable<int32> InputEnabledVariable;

	TAutoConsoleVariable<int32> InputNavigationVariable;

	TAutoConsoleVariable<int32> ShowDemoVariable;
};
