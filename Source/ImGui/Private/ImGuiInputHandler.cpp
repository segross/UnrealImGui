// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiInputHandler.h"

#include "ImGuiContextProxy.h"
#include "ImGuiModuleManager.h"

#include <Engine/Console.h>
#include <Input/Events.h>


FImGuiInputResponse UImGuiInputHandler::OnKeyDown(const FKeyEvent& KeyEvent)
{
	// Ignore console open events, so we don't block it from opening.
	if (KeyEvent.GetKey() == EKeys::Tilde)
	{
		return FImGuiInputResponse{ false, false };
	}

	// Ignore escape event, if they are not meant for ImGui control.
	if (KeyEvent.GetKey() == EKeys::Escape && !HasImGuiActiveItem())
	{
		return FImGuiInputResponse{ false, false };
	}

	return DefaultResponse();
}

bool UImGuiInputHandler::HasImGuiActiveItem() const
{
	FImGuiContextProxy* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex);
	return ContextProxy && ContextProxy->HasActiveItem();
}

void UImGuiInputHandler::Initialize(FImGuiModuleManager* InModuleManager, UGameViewportClient* InGameViewport, int32 InContextIndex)
{
	ModuleManager = InModuleManager;
	GameViewport = InGameViewport;
	ContextIndex = InContextIndex;
}
