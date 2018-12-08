// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiModuleSettings.h"

#include "ImGuiModuleCommands.h"
#include "ImGuiModuleProperties.h"


//====================================================================================================
// UImGuiSettings
//====================================================================================================

UImGuiSettings* UImGuiSettings::DefaultInstance = nullptr;

FSimpleMulticastDelegate UImGuiSettings::OnSettingsLoaded;

void UImGuiSettings::PostInitProperties()
{
	Super::PostInitProperties();

	if (SwitchInputModeKey_DEPRECATED.Key.IsValid() && !ToggleInput.Key.IsValid())
	{
		const FString ConfigFileName = GetDefaultConfigFilename();

		// Move value to the new property.
		ToggleInput = MoveTemp(SwitchInputModeKey_DEPRECATED);

		// Remove from configuration file entry for obsolete property.
		if (FConfigFile* ConfigFile = GConfig->Find(ConfigFileName, false))
		{
			if (FConfigSection* Section = ConfigFile->Find(TEXT("/Script/ImGui.ImGuiSettings")))
			{
				if (Section->Remove(TEXT("SwitchInputModeKey")))
				{
					ConfigFile->Dirty = true;
					GConfig->Flush(false, ConfigFileName);
				}
			}
		}

		// Add to configuration file entry for new property.
		UpdateSinglePropertyInConfigFile(
			UImGuiSettings::StaticClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UImGuiSettings, ToggleInput)),
			ConfigFileName);
	}

	if (IsTemplate())
	{
		DefaultInstance = this;
		OnSettingsLoaded.Broadcast();
	}
}

void UImGuiSettings::BeginDestroy()
{
	Super::BeginDestroy();

	if (DefaultInstance == this)
	{
		DefaultInstance = nullptr;
	}
}

//====================================================================================================
// FImGuiModuleSettings
//====================================================================================================

FImGuiModuleSettings::FImGuiModuleSettings(FImGuiModuleProperties& InProperties, FImGuiModuleCommands& InCommands)
	: Properties(InProperties)
	, Commands(InCommands)
{
#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FImGuiModuleSettings::OnPropertyChanged);
#endif

	// Delegate initializer to support settings loaded after this object creation (in stand-alone builds) and potential
	// reloading of settings.
	UImGuiSettings::OnSettingsLoaded.AddRaw(this, &FImGuiModuleSettings::UpdateSettings);

	// Call initializer to support settings already loaded (editor).
	UpdateSettings();
}

FImGuiModuleSettings::~FImGuiModuleSettings()
{

	UImGuiSettings::OnSettingsLoaded.RemoveAll(this);

#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
#endif
}

void FImGuiModuleSettings::UpdateSettings()
{
	if (UImGuiSettings* SettingsObject = UImGuiSettings::Get())
	{
		SetImGuiInputHandlerClass(SettingsObject->ImGuiInputHandlerClass);
		SetShareKeyboardInput(SettingsObject->bShareKeyboardInput);
		SetShareGamepadInput(SettingsObject->bShareGamepadInput);
		SetUseSoftwareCursor(SettingsObject->bUseSoftwareCursor);
		SetToggleInputKey(SettingsObject->ToggleInput);
	}
}

void FImGuiModuleSettings::SetImGuiInputHandlerClass(const FStringClassReference& ClassReference)
{
	if (ImGuiInputHandlerClass != ClassReference)
	{
		ImGuiInputHandlerClass = ClassReference;
		OnImGuiInputHandlerClassChanged.Broadcast(ClassReference);
	}
}

void FImGuiModuleSettings::SetShareKeyboardInput(bool bShare)
{
	if (bShareKeyboardInput != bShare)
	{
		bShareKeyboardInput = bShare;
		Properties.SetKeyboardInputShared(bShare);
	}
}

void FImGuiModuleSettings::SetShareGamepadInput(bool bShare)
{
	if (bShareGamepadInput != bShare)
	{
		bShareGamepadInput = bShare;
		Properties.SetGamepadInputShared(bShare);
	}
}

void FImGuiModuleSettings::SetUseSoftwareCursor(bool bUse)
{
	if (bUseSoftwareCursor != bUse)
	{
		bUseSoftwareCursor = bUse;
		OnUseSoftwareCursorChanged.Broadcast(bUse);
	}
}

void FImGuiModuleSettings::SetToggleInputKey(const FImGuiKeyInfo& KeyInfo)
{
	if (ToggleInputKey != KeyInfo)
	{
		ToggleInputKey = KeyInfo;
		Commands.SetKeyBinding(FImGuiModuleCommands::ToggleInput, ToggleInputKey);
	}
}

#if WITH_EDITOR

void FImGuiModuleSettings::OnPropertyChanged(class UObject* ObjectBeingModified, struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (ObjectBeingModified == UImGuiSettings::Get())
	{
		UpdateSettings();
	}
}

#endif // WITH_EDITOR
