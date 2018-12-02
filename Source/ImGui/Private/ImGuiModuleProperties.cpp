// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiModuleProperties.h"

#include "ImGuiSettings.h"


FImGuiModuleProperties::FImGuiModuleProperties()
{
	// Delegate initializer to support settings loaded after this object creation (in stand-alone builds) and potential
	// reloading of settings.
	UImGuiSettings::OnSettingsLoaded().AddRaw(this, &FImGuiModuleProperties::InitializeSettings);

	// Call initializer to support already loaded settings (editor).
	InitializeSettings();
}

FImGuiModuleProperties::~FImGuiModuleProperties()
{
	UImGuiSettings::OnSettingsLoaded().RemoveAll(this);
	UnregisterSettingsDelegates();
}

void FImGuiModuleProperties::InitializeSettings()
{
	if (GImGuiSettings)
	{
		RegisterSettingsDelegates();

		bKeyboardInputShared = GImGuiSettings->ShareKeyboardInput();
		bGamepadInputShared = GImGuiSettings->ShareGamepadInput();
	}
}

void FImGuiModuleProperties::RegisterSettingsDelegates()
{
	if (GImGuiSettings)
	{
		if (!GImGuiSettings->OnShareKeyboardInputChanged.IsBoundToObject(this))
		{
			GImGuiSettings->OnShareKeyboardInputChanged.AddRaw(this, &FImGuiModuleProperties::SetKeyboardInputShared);
		}
		if (!GImGuiSettings->OnShareGamepadInputChanged.IsBoundToObject(this))
		{
			GImGuiSettings->OnShareGamepadInputChanged.AddRaw(this, &FImGuiModuleProperties::SetGamepadInputShared);
		}
	}
}

void FImGuiModuleProperties::UnregisterSettingsDelegates()
{
	if (GImGuiSettings)
	{
		GImGuiSettings->OnShareKeyboardInputChanged.RemoveAll(this);
		GImGuiSettings->OnShareGamepadInputChanged.RemoveAll(this);
	}
}
