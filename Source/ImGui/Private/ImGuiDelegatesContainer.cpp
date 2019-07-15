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

int32 FImGuiDelegatesContainer::GetContextIndex(UWorld* World)
{
	return Utilities::GetWorldContextIndex(*World);
}

void FImGuiDelegatesContainer::Clear()
{
	WorldEarlyDebugDelegates.Empty();
	WorldDebugDelegates.Empty();
	MultiContextEarlyDebugDelegate.Clear();
	MultiContextDebugDelegate.Clear();
}
