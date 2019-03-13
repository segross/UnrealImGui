// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

// We build ImGui source code as part of this module. This is for convenience (no need to manually build libraries for
// different target platforms) but it also exposes the whole ImGui source for inspection, which can be pretty handy.
// Source files are included from Third Party directory, so we can wrap them in required by Unreal Build System headers
// without modifications in ImGui source code.
//
// We don't need to define IMGUI_API manually because it is already done for this module.

#if PLATFORM_XBOXONE
// Disable Win32 functions used in ImGui and not supported on XBox.
#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
#endif // PLATFORM_XBOXONE

#if WITH_EDITOR
// Global ImGui context pointer.
ImGuiContext* GImGuiContextPtr = nullptr;
// Handle to the global ImGui context pointer.
ImGuiContext** GImGuiContextPtrHandle = &GImGuiContextPtr;
// Get the global ImGui context pointer (GImGui) indirectly to allow redirections in obsolete modules.
#define GImGui (*GImGuiContextPtrHandle)
#endif // WITH_EDITOR

#if PLATFORM_WINDOWS
#include <AllowWindowsPlatformTypes.h>
#endif // PLATFORM_WINDOWS

#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui_widgets.cpp"
#include "misc/stl/imgui_stl.cpp"

#if PLATFORM_WINDOWS
#include <HideWindowsPlatformTypes.h>
#endif // PLATFORM_WINDOWS

#include "ImGuiInteroperability.h"


namespace ImGuiImplementation
{
#if WITH_EDITOR
	ImGuiContext** GetImGuiContextHandle()
	{
		return GImGuiContextPtrHandle;
	}

	void SetImGuiContextHandle(ImGuiContext** Handle)
	{
		GImGuiContextPtrHandle = Handle;
	}
#endif // WITH_EDITOR
}