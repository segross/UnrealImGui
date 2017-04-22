// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiDrawData.h"

#include <imgui.h>


class FImGuiInputState;

// Represents a single ImGui context. All the context updates should be done through this proxy. During update it
// broadcasts draw events to allow listeners draw their controls. After update it stores produced draw data.
// TODO: Add dynamically created contexts, so we can have a better support for multi-PIE.
class FImGuiContextProxy
{
public:

	FImGuiContextProxy();
	~FImGuiContextProxy();

	FImGuiContextProxy(const FImGuiContextProxy&) = delete;
	FImGuiContextProxy& operator=(const FImGuiContextProxy&) = delete;

	FImGuiContextProxy(FImGuiContextProxy&&) = delete;
	FImGuiContextProxy& operator=(FImGuiContextProxy&&) = delete;

	// Get draw data from the last frame.
	const TArray<FImGuiDrawList>& GetDrawData() const { return DrawLists; }

	// Delegate called right before ending the frame to allows listeners draw their controls.
	FSimpleMulticastDelegate& OnDraw() { return DrawEvent; }

	// Tick to advance context to the next frame.
	// @param DeltaSeconds - Time delta in seconds (will be passed to ImGui)
	// @param InputState - Input state for ImGui IO or null if there is no input for this context
	void Tick(float DeltaSeconds, const FImGuiInputState* InputState = nullptr);

private:

	void BeginFrame(float DeltaTime = 1.f / 60.f, const FImGuiInputState* InputState = nullptr);
	void EndFrame();

	void UpdateDrawData(ImDrawData* DrawData);

	TArray<FImGuiDrawList> DrawLists;

	FSimpleMulticastDelegate DrawEvent;

	bool bIsFrameStarted = false;
};
