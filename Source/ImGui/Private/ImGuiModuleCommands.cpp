// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiModuleCommands.h"

#include "ImGuiModuleManager.h"
#include "ImGuiSettings.h"
#include "Utilities/DebugExecBindings.h"


namespace CommandNames
{
	namespace
	{
		const TCHAR* ToggleInput = TEXT("ImGui.ToggleInput");
		const TCHAR* ToggleKeyboardNavigation = TEXT("ImGui.ToggleKeyboardNavigation");
		const TCHAR* ToggleGamepadNavigation = TEXT("ImGui.ToggleGamepadNavigation");
		const TCHAR* ToggleKeyboardInputSharing = TEXT("ImGui.ToggleKeyboardInputSharing");
		const TCHAR* ToggleGamepadInputSharing = TEXT("ImGui.ToggleGamepadInputSharing");
		const TCHAR* ToggleDemo = TEXT("ImGui.ToggleDemo");
	}
}

FImGuiModuleCommands::FImGuiModuleCommands(FImGuiModuleManager& InModuleManager)
	: ModuleManager(InModuleManager)
	, ToggleInputCommand(CommandNames::ToggleInput,
		TEXT("Toggle ImGui input mode."),
		FConsoleCommandDelegate::CreateRaw(this, &FImGuiModuleCommands::ToggleInput))
	, ToggleKeyboardNavigationCommand(CommandNames::ToggleKeyboardNavigation,
		TEXT("Toggle ImGui keyboard navigation."),
		FConsoleCommandDelegate::CreateRaw(this, &FImGuiModuleCommands::ToggleKeyboardNavigation))
	, ToggleGamepadNavigationCommand(CommandNames::ToggleGamepadNavigation,
		TEXT("Toggle ImGui gamepad navigation."),
		FConsoleCommandDelegate::CreateRaw(this, &FImGuiModuleCommands::ToggleGamepadNavigation))
	, ToggleKeyboardInputSharingCommand(CommandNames::ToggleKeyboardInputSharing,
		TEXT("Toggle ImGui keyboard input sharing."),
		FConsoleCommandDelegate::CreateRaw(this, &FImGuiModuleCommands::ToggleKeyboardInputSharing))
	, ToggleGamepadInputSharingCommand(CommandNames::ToggleGamepadInputSharing,
		TEXT("Toggle ImGui gamepad input sharing."),
		FConsoleCommandDelegate::CreateRaw(this, &FImGuiModuleCommands::ToggleGamepadInputSharing))
	, ToggleDemoCommand(CommandNames::ToggleDemo,
		TEXT("Toggle ImGui demo."),
		FConsoleCommandDelegate::CreateRaw(this, &FImGuiModuleCommands::ToggleDemo))
{
	// Delegate initializer to support settings loaded after this object creation (in stand-alone builds) and potential
	// reloading of settings.
	UImGuiSettings::OnSettingsLoaded().AddRaw(this, &FImGuiModuleCommands::InitializeSettings);

	// Call initializer to support settings already loaded (editor).
	InitializeSettings();
}

FImGuiModuleCommands::~FImGuiModuleCommands()
{
	UImGuiSettings::OnSettingsLoaded().RemoveAll(this);
	UnregisterSettingsDelegates();
}

void FImGuiModuleCommands::InitializeSettings()
{
	RegisterSettingsDelegates();

	// We manually update key bindings based on ImGui settings rather than using input configuration. This works out
	// of the box in packed and staged builds and it helps to avoid ambiguities where ImGui settings are stored.
	UpdateToggleInputKeyBinding();
}

void FImGuiModuleCommands::RegisterSettingsDelegates()
{
	if (GImGuiSettings && !GImGuiSettings->OnToggleInputKeyChanged.IsBoundToObject(this))
	{
		GImGuiSettings->OnToggleInputKeyChanged.AddRaw(this, &FImGuiModuleCommands::UpdateToggleInputKeyBinding);
	}
}

void FImGuiModuleCommands::UnregisterSettingsDelegates()
{
	if (GImGuiSettings)
	{
		GImGuiSettings->OnToggleInputKeyChanged.RemoveAll(this);
	}
}

void FImGuiModuleCommands::UpdateToggleInputKeyBinding()
{
	if (GImGuiSettings)
	{
		DebugExecBindings::UpdatePlayerInputs(GImGuiSettings->GetToggleInputKey(), CommandNames::ToggleInput);
	}
}

void FImGuiModuleCommands::ToggleInput()
{
	ModuleManager.GetProperties().ToggleInput();
}

void FImGuiModuleCommands::ToggleKeyboardNavigation()
{
	ModuleManager.GetProperties().ToggleKeyboardNavigation();
}

void FImGuiModuleCommands::ToggleGamepadNavigation()
{
	ModuleManager.GetProperties().ToggleGamepadNavigation();
}

void FImGuiModuleCommands::ToggleKeyboardInputSharing()
{
	ModuleManager.GetProperties().ToggleKeyboardInputSharing();
}

void FImGuiModuleCommands::ToggleGamepadInputSharing()
{
	ModuleManager.GetProperties().ToggleGamepadInputSharing();
}

void FImGuiModuleCommands::ToggleDemo()
{
	ModuleManager.GetProperties().ToggleDemo();
}
