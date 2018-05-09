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

#if PLATFORM_WINDOWS
#include <AllowWindowsPlatformTypes.h>
#endif // PLATFORM_WINDOWS

#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"

#if PLATFORM_WINDOWS
#include <HideWindowsPlatformTypes.h>
#endif // PLATFORM_WINDOWS


#include "ImGuiInteroperability.h"

namespace ImGuiImplementation
{
	bool GetCursorData(ImGuiMouseCursor CursorType, FVector2D& OutSize, FVector2D& OutUVMin, FVector2D& OutUVMax, FVector2D& OutOutlineUVMin, FVector2D& OutOutlineUVMax)
	{
		ImFontAtlas* FontAtlas = ImGui::GetIO().Fonts;
		ImVec2 Offset, Size, UV[4];
		if (FontAtlas && FontAtlas->GetMouseCursorTexData(CursorType, &Offset, &Size, &UV[0], &UV[2]))
		{
			using namespace ImGuiInterops;
			OutSize = ToVector2D(Size);
			OutUVMin = ToVector2D(UV[0]);
			OutUVMax = ToVector2D(UV[1]);
			OutOutlineUVMin = ToVector2D(UV[2]);
			OutOutlineUVMax = ToVector2D(UV[3]);
			return true;
		}
		else
		{
			return false;
		}
	}
}