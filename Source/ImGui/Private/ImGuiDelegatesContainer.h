// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <Containers/Map.h>
#include <Delegates/Delegate.h>


#if WITH_EDITOR
struct FImGuiDelegatesContainerHandle;
#endif

struct FImGuiDelegatesContainer
{
public:

	// Get the current instance (can change during hot-reloading).
	static FImGuiDelegatesContainer& Get();

#if WITH_EDITOR
	// Get the handle to the container instance (can attach to other handles in hot-reloaded modules).
	static FImGuiDelegatesContainerHandle& GetHandle();

	// Redirect to the other container and if this one is still active move its data to the other one.
	static void MoveContainer(FImGuiDelegatesContainerHandle& OtherContainerHandle);
#endif

	// Get delegate to ImGui world early debug event from known world instance.
	FSimpleMulticastDelegate& OnWorldEarlyDebug(UWorld* World) { return OnWorldEarlyDebug(GetContextIndex(World)); }

	// Get delegate to ImGui world early debug event from known context index.
	FSimpleMulticastDelegate& OnWorldEarlyDebug(int32 ContextIndex) { return WorldEarlyDebugDelegates.FindOrAdd(ContextIndex); }

	// Get delegate to ImGui multi-context early debug event.
	FSimpleMulticastDelegate& OnMultiContextEarlyDebug() { return MultiContextEarlyDebugDelegate; }

	// Get delegate to ImGui world debug event from known world instance.
	FSimpleMulticastDelegate& OnWorldDebug(UWorld* World) { return OnWorldDebug(GetContextIndex(World)); }

	// Get delegate to ImGui world debug event from known context index.
	FSimpleMulticastDelegate& OnWorldDebug(int32 ContextIndex) { return WorldDebugDelegates.FindOrAdd(ContextIndex); }

	// Get delegate to ImGui multi-context debug event.
	FSimpleMulticastDelegate& OnMultiContextDebug() { return MultiContextDebugDelegate; }

private:

	int32 GetContextIndex(UWorld* World);

	void Clear();

	TMap<int32, FSimpleMulticastDelegate> WorldEarlyDebugDelegates;
	TMap<int32, FSimpleMulticastDelegate> WorldDebugDelegates;
	FSimpleMulticastDelegate MultiContextEarlyDebugDelegate;
	FSimpleMulticastDelegate MultiContextDebugDelegate;
};
