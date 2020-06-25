// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiDelegates.h"
#include "ImGuiDelegatesContainer.h"

#include <Engine/World.h>


FSimpleMulticastDelegate& FImGuiDelegates::OnWorldEarlyDebug()
{
	return OnWorldEarlyDebug(GWorld);
}

FSimpleMulticastDelegate& FImGuiDelegates::OnWorldEarlyDebug(UWorld* World)
{
	return FImGuiDelegatesContainer::Get().OnWorldEarlyDebug(World);
}

FSimpleMulticastDelegate& FImGuiDelegates::OnMultiContextEarlyDebug()
{
	return FImGuiDelegatesContainer::Get().OnMultiContextEarlyDebug();
}

FSimpleMulticastDelegate& FImGuiDelegates::OnWorldDebug()
{
	return OnWorldDebug(GWorld);
}

FSimpleMulticastDelegate& FImGuiDelegates::OnWorldDebug(UWorld* World)
{
	return FImGuiDelegatesContainer::Get().OnWorldDebug(World);
}

FSimpleMulticastDelegate& FImGuiDelegates::OnMultiContextDebug()
{
	return FImGuiDelegatesContainer::Get().OnMultiContextDebug();
}
