// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiDrawData.h"

#include <imgui.h>


class FImGuiInputState;

// Represents a single ImGui context. All the context updates should be done through this proxy. During update it
// broadcasts draw events to allow listeners draw their controls. After update it stores draw data.
class FImGuiContextProxy
{
public:

	FImGuiContextProxy();
	~FImGuiContextProxy();

	FImGuiContextProxy(const FImGuiContextProxy&) = delete;
	FImGuiContextProxy& operator=(const FImGuiContextProxy&) = delete;

	FImGuiContextProxy(FImGuiContextProxy&& Other);
	FImGuiContextProxy& operator=(FImGuiContextProxy&& Other);

	// Get draw data from the last frame.
	const TArray<FImGuiDrawList>& GetDrawData() const { return DrawLists; }

	// Get input state used by this context.
	const FImGuiInputState* GetInputState() const { return InputState; }

	// Set input state to be used by this context.
	void SetInputState(const FImGuiInputState* SourceInputState) { InputState = SourceInputState; }

	// Is this context the current ImGui context.
	bool IsCurrentContext() const { return ImGui::GetCurrentContext() == Context; }

	// Set this context as current ImGui context.
	void SetAsCurrent() { ImGui::SetCurrentContext(Context); }

	bool HasActiveItem() const { return bHasActiveItem; }

	// Delegate called right before ending the frame to allows listeners draw their controls.
	FSimpleMulticastDelegate& OnDraw() { return DrawEvent; }

	// Tick to advance context to the next frame.
	void Tick(float DeltaSeconds);

private:

	void BeginFrame(float DeltaTime = 1.f / 60.f);
	void EndFrame();

	void UpdateDrawData(ImDrawData* DrawData);

	ImGuiContext* Context = nullptr;

	bool bHasActiveItem = false;

	bool bIsFrameStarted = false;
	FSimpleMulticastDelegate DrawEvent;
	const FImGuiInputState* InputState = nullptr;

	TArray<FImGuiDrawList> DrawLists;
};
