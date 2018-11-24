// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiModuleProperties.h"


FImGuiModuleProperties& FImGuiModuleProperties::Get()
{
	static FImGuiModuleProperties Instance;
	return Instance;
}

FImGuiModuleProperties::FImGuiModuleProperties()
	: InputEnabledVariable(TEXT("ImGui.InputEnabled"), 0,
		TEXT("Enable or disable ImGui input mode.\n")
		TEXT("0: disabled (default)\n")
		TEXT("1: enabled, input is routed to ImGui and with a few exceptions is consumed"),
		ECVF_Default)
	, InputNavigationVariable(TEXT("ImGui.InputNavigation"), 0,
		TEXT("EXPERIMENTAL Set ImGui navigation mode.\n")
		TEXT("0: navigation is disabled\n")
		TEXT("1: keyboard navigation\n")
		TEXT("2: gamepad navigation (gamepad input is consumed)\n")
		TEXT("3: keyboard and gamepad navigation (gamepad input is consumed)"),
		ECVF_Default)
	, ShowDemoVariable(TEXT("ImGui.ShowDemo"), 0,
		TEXT("Show ImGui demo.\n")
		TEXT("0: disabled (default)\n")
		TEXT("1: enabled."),
		ECVF_Default)
{
}

bool FImGuiModuleProperties::IsInputEnabled() const
{
	return InputEnabledVariable->GetInt() > 0;
}

void FImGuiModuleProperties::SetInputEnabled(bool bEnabled, EConsoleVariableFlags SetBy)
{
	InputEnabledVariable->Set(bEnabled ? 1 : 0, SetBy);
}

void FImGuiModuleProperties::ToggleInput(EConsoleVariableFlags SetBy)
{
	SetInputEnabled(!IsInputEnabled(), SetBy);
}

bool FImGuiModuleProperties::IsKeyboardNavigationEnabled() const
{
	return (InputNavigationVariable->GetInt() & 1) != 0;
}

bool FImGuiModuleProperties::IsGamepadNavigationEnabled() const
{
	return (InputNavigationVariable->GetInt() & 2) != 0;
}

bool FImGuiModuleProperties::ShowDemo() const
{
	return ShowDemoVariable->GetInt() > 0;
}

void FImGuiModuleProperties::SetShowDemo(bool bEnabled, EConsoleVariableFlags SetBy)
{
	ShowDemoVariable->Set(bEnabled ? 1 : 0, SetBy);
}

void FImGuiModuleProperties::ToggleDemo(EConsoleVariableFlags SetBy)
{
	SetShowDemo(!ShowDemo(), SetBy);
}
