// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

struct FImGuiContextHandle;

// Gives access to selected ImGui implementation features.
namespace ImGuiImplementation
{
#if WITH_EDITOR
	// Get the handle to the ImGui Context pointer.
	FImGuiContextHandle& GetContextHandle();

	// Set the ImGui Context pointer handle.
	void SetParentContextHandle(FImGuiContextHandle& Parent);
#endif // WITH_EDITOR
}
