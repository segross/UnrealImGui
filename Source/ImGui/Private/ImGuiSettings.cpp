// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiSettings.h"
#include "Utilities/DebugExecBindings.h"


UImGuiSettings::UImGuiSettings()
{
#if WITH_EDITOR
	RegisterPropertyChangedDelegate();
#endif
}

UImGuiSettings::~UImGuiSettings()
{
#if WITH_EDITOR
	UnregisterPropertyChangedDelegate();
#endif
}

namespace Commands
{
	extern const TCHAR* SwitchInputMode;
}

void UImGuiSettings::PostInitProperties()
{
	Super::PostInitProperties();

	// Instead of saving binding to input config, we manually update DebugExecBindings from here. This has an advantage
	// that there is no ambiguity where settings are stored and more importantly, it works out of the box in packed
	// and staged builds.  
	DebugExecBindings::UpdatePlayerInputs(SwitchInputModeKey, Commands::SwitchInputMode);
}

#if WITH_EDITOR

void UImGuiSettings::RegisterPropertyChangedDelegate()
{
	if (!FCoreUObjectDelegates::OnObjectPropertyChanged.IsBoundToObject(this))
	{
		FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UImGuiSettings::OnPropertyChanged);
	}
}

void UImGuiSettings::UnregisterPropertyChangedDelegate()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}

void UImGuiSettings::OnPropertyChanged(class UObject* ObjectBeingModified, struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (ObjectBeingModified == this)
	{
		const FName UpdatedPropertyName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

		if (UpdatedPropertyName == GET_MEMBER_NAME_CHECKED(UImGuiSettings, ImGuiInputHandlerClass))
		{
			OnImGuiInputHandlerClassChanged.Broadcast();
		}
		else if (UpdatedPropertyName == GET_MEMBER_NAME_CHECKED(UImGuiSettings, SwitchInputModeKey))
		{
			DebugExecBindings::UpdatePlayerInputs(SwitchInputModeKey, Commands::SwitchInputMode);
		}
	}
}

#endif // WITH_EDITOR
