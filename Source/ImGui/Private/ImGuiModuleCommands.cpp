// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiModuleCommands.h"

#include "ImGuiSettings.h"
#include "Utilities/DebugExecBindings.h"


namespace CommandNames
{
	namespace
	{
		const TCHAR* ToggleInput = TEXT("ImGui.ToggleInput");
	}
}

FImGuiModuleCommands::FImGuiModuleCommands()
	: ToggleInputCommand(CommandNames::ToggleInput,
		TEXT("Switch ImGui input mode."),
		FConsoleCommandDelegate::CreateRaw(this, &FImGuiModuleCommands::ToggleInput))
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
	FImGuiModuleProperties::Get().ToggleInput(ECVF_SetByConsole);
}
