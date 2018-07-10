// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiSettings.h"


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
		static const FName ImGuiInputHandlerPropertyName = GET_MEMBER_NAME_CHECKED(UImGuiSettings, ImGuiInputHandlerClass);

		const FName UpdatedPropertyName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
		if (UpdatedPropertyName == ImGuiInputHandlerPropertyName)
		{
			OnImGuiInputHandlerClassChanged.Broadcast();
		}
	}
}

#endif // WITH_EDITOR
