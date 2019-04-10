// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiDelegates.h"
#include "ImGuiDelegatesContainer.h"


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
