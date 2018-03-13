// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiDrawData.h"

#include <ICursor.h>

#include <imgui.h>

#include <string>


class FImGuiInputState;

// Represents a single ImGui context. All the context updates should be done through this proxy. During update it
// broadcasts draw events to allow listeners draw their controls. After update it stores draw data.
class FImGuiContextProxy
{
public:

	FImGuiContextProxy(const FString& Name);
	~FImGuiContextProxy();

	FImGuiContextProxy(const FImGuiContextProxy&) = delete;
	FImGuiContextProxy& operator=(const FImGuiContextProxy&) = delete;

	FImGuiContextProxy(FImGuiContextProxy&& Other);
	FImGuiContextProxy& operator=(FImGuiContextProxy&& Other);

	// Get the name of this context.
	const FString& GetName() const { return Name; }

	// Get draw data from the last frame.
	const TArray<FImGuiDrawList>& GetDrawData() const { return DrawLists; }

	// Get input state used by this context.
	const FImGuiInputState* GetInputState() const { return InputState; }

	// Set input state to be used by this context.
	void SetInputState(const FImGuiInputState* SourceInputState) { InputState = SourceInputState; }

	// If context is currently using input state to remove then remove that binding.
	void RemoveInputState(const FImGuiInputState* InputStateToRemove) { if (InputState == InputStateToRemove) InputState = nullptr; }

	// Is this context the current ImGui context.
	bool IsCurrentContext() const { return ImGui::GetCurrentContext() == Context; }

	// Set this context as current ImGui context.
	void SetAsCurrent() { ImGui::SetCurrentContext(Context); }

	bool HasActiveItem() const { return bHasActiveItem; }

	EMouseCursor::Type GetMouseCursor() const { return MouseCursor;  }

	// Delegate called right before ending the frame to allows listeners draw their controls.
	FSimpleMulticastDelegate& OnDraw() { return DrawEvent; }

	// Tick to advance context to the next frame.
	// @param SharedDrawEvent - Shared draw event provided from outside to be called right after context own event 
	void Tick(float DeltaSeconds, FSimpleMulticastDelegate* SharedDrawEvent = nullptr);

private:

	void BeginFrame(float DeltaTime = 1.f / 60.f);
	void EndFrame();

	void UpdateDrawData(ImDrawData* DrawData);

	ImGuiContext* Context = nullptr;

	bool bHasActiveItem = false;
	EMouseCursor::Type MouseCursor = EMouseCursor::None;

	bool bIsFrameStarted = false;
	FSimpleMulticastDelegate DrawEvent;
	const FImGuiInputState* InputState = nullptr;

	TArray<FImGuiDrawList> DrawLists;

	FString Name;
	std::string IniFilename;
};
