// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiDrawData.h"
#include "ImGuiInputState.h"
#include "Utilities/WorldContextIndex.h"

#include <GenericPlatform/ICursor.h>

#include <imgui.h>

#include <string>


// Represents a single ImGui context. All the context updates should be done through this proxy. During update it
// broadcasts draw events to allow listeners draw their controls. After update it stores draw data.
class FImGuiContextProxy
{
public:

	FImGuiContextProxy(const FString& Name, int32 InContextIndex, FSimpleMulticastDelegate* InSharedDrawEvent, ImFontAtlas* InFontAtlas);
	~FImGuiContextProxy();

	FImGuiContextProxy(const FImGuiContextProxy&) = delete;
	FImGuiContextProxy& operator=(const FImGuiContextProxy&) = delete;

	FImGuiContextProxy(FImGuiContextProxy&&) = delete;
	FImGuiContextProxy& operator=(FImGuiContextProxy&&) = delete;

	// Get the name of this context.
	const FString& GetName() const { return Name; }

	// Get draw data from the last frame.
	const TArray<FImGuiDrawList>& GetDrawData() const { return DrawLists; }

	// Get input state used by this context.
	FImGuiInputState& GetInputState() { return InputState; }
	const FImGuiInputState& GetInputState() const { return InputState; }

	// Is this context the current ImGui context.
	bool IsCurrentContext() const { return ImGui::GetCurrentContext() == Context; }

	// Set this context as current ImGui context.
	void SetAsCurrent() { ImGui::SetCurrentContext(Context); }

	// Context display size (read once per frame during context update).
	const FVector2D& GetDisplaySize() const { return DisplaySize; }

	// Whether this context has an active item (read once per frame during context update).
	bool HasActiveItem() const { return bHasActiveItem; }

	// Whether this context has mouse hovering any window (read once per frame during context update).
	bool IsMouseHoveringAnyWindow() const { return bIsMouseHoveringAnyWindow; }

	// Cursor type desired by this context (updated once per frame during context update).
	EMouseCursor::Type GetMouseCursor() const { return MouseCursor;  }

	// Delegate called right before ending the frame to allows listeners draw their controls.
	FSimpleMulticastDelegate& OnDraw() { return DrawEvent; }

	// Call early debug events to allow listeners draw their debug widgets.
	void DrawEarlyDebug();

	// Call debug events to allow listeners draw their debug widgets.
	void DrawDebug();

	// Tick to advance context to the next frame. Only one call per frame will be processed.
	void Tick(float DeltaSeconds);

private:

	void BeginFrame(float DeltaTime = 1.f / 60.f);
	void EndFrame();

	void UpdateDrawData(ImDrawData* DrawData);

	void BroadcastWorldEarlyDebug();
	void BroadcastMultiContextEarlyDebug();

	void BroadcastWorldDebug();
	void BroadcastMultiContextDebug();

	ImGuiContext* Context;

	FVector2D DisplaySize = FVector2D::ZeroVector;

	EMouseCursor::Type MouseCursor = EMouseCursor::None;
	bool bHasActiveItem = false;
	bool bIsMouseHoveringAnyWindow = false;

	bool bIsFrameStarted = false;
	bool bIsDrawEarlyDebugCalled = false;
	bool bIsDrawDebugCalled = false;

	FImGuiInputState InputState;

	TArray<FImGuiDrawList> DrawLists;

	FString Name;
	int32 ContextIndex = Utilities::INVALID_CONTEXT_INDEX;

	uint32 LastFrameNumber = 0;

	FSimpleMulticastDelegate DrawEvent;
	FSimpleMulticastDelegate* SharedDrawEvent = nullptr;

	std::string IniFilename;
};
