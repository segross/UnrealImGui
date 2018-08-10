// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <imgui.h>


// Gives access to selected ImGui implementation features.
namespace ImGuiImplementation
{
	// Get specific cursor data.
	bool GetCursorData(ImGuiMouseCursor CursorType, FVector2D& OutSize, FVector2D& OutUVMin, FVector2D& OutUVMax, FVector2D& OutOutlineUVMin, FVector2D& OutOutlineUVMax);

#if WITH_EDITOR
	// Get the handle to the ImGui Context pointer.
	ImGuiContext** GetImGuiContextHandle();

	// Set the ImGui Context pointer handle.
	void SetImGuiContextHandle(ImGuiContext** Handle);
#endif // WITH_EDITOR
}
