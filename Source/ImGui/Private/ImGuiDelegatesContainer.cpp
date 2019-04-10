// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiDelegatesContainer.h"

#include "Utilities/WorldContextIndex.h"


FImGuiDelegatesContainer FImGuiDelegatesContainer::DefaultInstance;

FImGuiDelegatesContainer* FImGuiDelegatesContainer::InstancePtr = &FImGuiDelegatesContainer::DefaultInstance;

void FImGuiDelegatesContainer::MoveContainer(FImGuiDelegatesContainer& Dst)
{
	// Only move data if pointer points to default instance, otherwise our data has already been moved and we only
	// keep pointer to a more recent version.
	if (InstancePtr == &DefaultInstance)
	{
		Dst = MoveTemp(DefaultInstance);
		DefaultInstance.Clear();
	}

	// Update pointer to the most recent version.
	InstancePtr = &Dst;
}

FSimpleMulticastDelegate& FImGuiDelegatesContainer::OnWorldDebug(UWorld* World)
{
	return OnWorldDebug(Utilities::GetWorldContextIndex(*World));
}

void FImGuiDelegatesContainer::Clear()
{
	WorldDelegates.Empty();
	MultiContextDelegate.Clear();
}
