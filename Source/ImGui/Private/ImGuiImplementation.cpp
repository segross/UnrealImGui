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

#include "ImGuiModule.h"
#include "Utilities/RedirectingHandle.h"

// Redirecting handle which will automatically bind to another one, if a different instance of the module is loaded.
struct FImGuiContextHandle : public Utilities::TRedirectingHandle<ImGuiContext*>
{
	FImGuiContextHandle(ImGuiContext*& InDefaultContext)
		: Utilities::TRedirectingHandle<ImGuiContext*>(InDefaultContext)
	{
		if (FImGuiModule* Module = FModuleManager::GetModulePtr<FImGuiModule>("ImGui"))
		{
			SetParent(Module->ImGuiContextHandle);
		}
	}
};

static ImGuiContext* ImGuiContextPtr = nullptr;
static FImGuiContextHandle ImGuiContextPtrHandle(ImGuiContextPtr);

// Get the global ImGui context pointer (GImGui) indirectly to allow redirections in obsolete modules.
#define GImGui (ImGuiContextPtrHandle.Get())
#endif // WITH_EDITOR

#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui_widgets.cpp"

#include "imgui_tables.cpp"
#include "implot.cpp"
#include "implot_items.cpp"
#include "implot_demo.cpp"

#if PLATFORM_WINDOWS
#include <Windows/HideWindowsPlatformTypes.h>
#endif // PLATFORM_WINDOWS

#include "ImGuiInteroperability.h"


namespace ImGuiImplementation
{
#if WITH_EDITOR
	FImGuiContextHandle& GetContextHandle()
	{
		return ImGuiContextPtrHandle;
	}

	void SetParentContextHandle(FImGuiContextHandle& Parent)
	{
		ImGuiContextPtrHandle.SetParent(&Parent);
	}
#endif // WITH_EDITOR
}
