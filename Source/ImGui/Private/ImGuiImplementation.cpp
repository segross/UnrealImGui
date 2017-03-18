// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

// We build ImGui source code as part of this module. This is for convenience (no need to manually build libraries for
// different target platforms) but it also exposes the whole ImGui source for inspection, which can be pretty handy.
// Source files are included from Third Party directory, so we can wrap them in required by Unreal Build System headers
// without modifications in ImGui source code.
//
// We don't need to define IMGUI_API manually because it is already done for this module.

#if PLATFORM_WINDOWS
#include <AllowWindowsPlatformTypes.h>
#endif // PLATFORM_WINDOWS

#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"

#if PLATFORM_WINDOWS
#include <HideWindowsPlatformTypes.h>
#endif // PLATFORM_WINDOWS
