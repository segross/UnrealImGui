// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <imgui.h>


// Gives access to selected ImGui implementation features.
namespace ImGuiImplementation
{
#if WITH_EDITOR
	// Get the handle to the ImGui Context pointer.
	ImGuiContext** GetImGuiContextHandle();

	// Set the ImGui Context pointer handle.
	void SetImGuiContextHandle(ImGuiContext** Handle);
#endif // WITH_EDITOR
}
