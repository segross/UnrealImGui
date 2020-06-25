// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiImplementation.h"

#include <CoreMinimal.h>

// For convenience and easy access to the ImGui source code, we build it as part of this module.
// We don't need to define IMGUI_API manually because it is already done for this module.

#if PLATFORM_XBOXONE
// Disable Win32 functions used in ImGui and not supported on XBox.
#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
#endif // PLATFORM_XBOXONE

#if PLATFORM_WINDOWS
#include <Windows/AllowWindowsPlatformTypes.h>
#endif // PLATFORM_WINDOWS

#if WITH_EDITOR
// Global ImGui context pointer.
ImGuiContext* GImGuiContextPtr = nullptr;
// Handle to the global ImGui context pointer.
ImGuiContext** GImGuiContextPtrHandle = &GImGuiContextPtr;
// Get the global ImGui context pointer (GImGui) indirectly to allow redirections in obsolete modules.
#define GImGui (*GImGuiContextPtrHandle)
#endif // WITH_EDITOR

#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui_widgets.cpp"

#if PLATFORM_WINDOWS
#include <Windows/HideWindowsPlatformTypes.h>
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
