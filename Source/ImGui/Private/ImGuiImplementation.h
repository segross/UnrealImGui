// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

struct FImGuiContextHandle;
struct ImFontAtlas;
struct ImGuiIO;

// Gives access to selected ImGui implementation features.
namespace ImGuiImplementation
{
	// Advance an externally owned atlas once per Unreal frame, shared by all PIE contexts.
	void UpdateFontAtlas(ImFontAtlas& Atlas);
	// Cancel queued and already consumed input for disabled devices in this IO's context.
	void ResetInput(ImGuiIO& IO, bool bKeyboard, bool bMouse, bool bGamepad);
#if WITH_EDITOR
	// Get the handle to the ImGui Context pointer.
	FImGuiContextHandle& GetContextHandle();

	// Set the ImGui Context pointer handle.
	void SetParentContextHandle(FImGuiContextHandle& Parent);
#endif // WITH_EDITOR
}
