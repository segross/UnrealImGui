// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiDelegatesContainer.h"

#include "ImGuiModule.h"
#include "Utilities/WorldContextIndex.h"


#if !WITH_EDITOR
//
// Non-editor version without container redirection
//

FImGuiDelegatesContainer& FImGuiDelegatesContainer::Get()
{
	static FImGuiDelegatesContainer* Container = new FImGuiDelegatesContainer();
	return *Container;
}

#endif // !WITH_EDITOR


#if WITH_EDITOR
//
// Editor version supporting container redirection needed for hot-reloading
//

#include "Utilities/RedirectingHandle.h"

// Redirecting handle which will always bind to a container from the currently loaded module.
struct FImGuiDelegatesContainerHandle : Utilities::TRedirectingHandle<FImGuiDelegatesContainer>
{
	FImGuiDelegatesContainerHandle(FImGuiDelegatesContainer& InDefaultContainer)
		: Utilities::TRedirectingHandle<FImGuiDelegatesContainer>(InDefaultContainer)
	{
		if (FImGuiModule* Module = FModuleManager::GetModulePtr<FImGuiModule>("ImGui"))
		{
			SetParent(Module->DelegatesContainerHandle);
		}
	}
};

FImGuiDelegatesContainer& FImGuiDelegatesContainer::Get()
{
	return GetHandle().Get();
}

FImGuiDelegatesContainerHandle& FImGuiDelegatesContainer::GetHandle()
{
	struct FContainerInstance
	{
		FContainerInstance() : Container(), Handle(Container) {}
		FImGuiDelegatesContainer Container;
		FImGuiDelegatesContainerHandle Handle;
	};
	static FContainerInstance* Instance = new FContainerInstance();
	return Instance->Handle;
}

void FImGuiDelegatesContainer::MoveContainer(FImGuiDelegatesContainerHandle& OtherContainerHandle)
{
	// Only move data if pointer points to default instance, otherwise our data has already been moved and we only
	// keep pointer to a more recent version.
	if (GetHandle().IsDefault())
	{
		OtherContainerHandle.Get() = MoveTemp(GetHandle().Get());
		GetHandle().Get().Clear();
	}

	// Update pointer to the most recent version.
	GetHandle().SetParent(&OtherContainerHandle);
}

#endif // WITH_EDITOR


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
