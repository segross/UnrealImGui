// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiSettings.h"


UImGuiSettings* GImGuiSettings = nullptr;

FSimpleMulticastDelegate& UImGuiSettings::OnSettingsLoaded()
{
	static FSimpleMulticastDelegate Instance;
	return Instance;
}


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

void UImGuiSettings::PostInitProperties()
{
	Super::PostInitProperties();

	if (IsTemplate())
	{
		GImGuiSettings = this;
		OnSettingsLoaded().Broadcast();
	}
}

void UImGuiSettings::BeginDestroy()
{
	Super::BeginDestroy();

	if (GImGuiSettings == this)
	{
		GImGuiSettings = nullptr;
	}
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
			OnSwitchInputModeKeyChanged.Broadcast();
		}
		else if (UpdatedPropertyName == GET_MEMBER_NAME_CHECKED(UImGuiSettings, bUseSoftwareCursor))
		{
			OnSoftwareCursorChanged.Broadcast();
		}
	}
}

#endif // WITH_EDITOR
