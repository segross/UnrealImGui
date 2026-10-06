// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiImplementation.h"

#define IMGUI_DEFINE_MATH_OPERATORS

#include <CoreMinimal.h>

// Clipboard integration is provided through Unreal's platform API in ImGuiContextProxy.
#define IMGUI_DISABLE_WIN32_FUNCTIONS

// For convenience and easy access to the ImGui source code, we build it as part of this module.
// We don't need to define IMGUI_API manually because it is already done for this module.

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

#if PLATFORM_WINDOWS
#include <Windows/HideWindowsPlatformTypes.h>
#endif // PLATFORM_WINDOWS

#include "ImGuiInteroperability.h"


namespace ImGuiImplementation
{
	void ResetInput(ImGuiIO& IO, bool bKeyboard, bool bMouse, bool bGamepad)
	{
		if (!bKeyboard && !bMouse && !bGamepad) return;
		ImGuiContext& InputContext = *IO.Ctx;
		int WriteIndex = 0;
		for (const ImGuiInputEvent& Event : InputContext.InputEventsQueue)
		{
			bool bDiscard = bKeyboard && Event.Type == ImGuiInputEventType_Text;
			if (Event.Type == ImGuiInputEventType_Key)
			{
				bDiscard |= ImGui::IsGamepadKey(Event.Key.Key) ? bGamepad : bKeyboard;
			}
			bDiscard |= bMouse && (Event.Type == ImGuiInputEventType_MousePos || Event.Type == ImGuiInputEventType_MouseButton || Event.Type == ImGuiInputEventType_MouseWheel);
			if (!bDiscard) InputContext.InputEventsQueue[WriteIndex++] = Event;
		}
		InputContext.InputEventsQueue.resize(WriteIndex);
		for (int Key = ImGuiKey_NamedKey_BEGIN; Key < ImGuiKey_NamedKey_END; ++Key)
		{
			const ImGuiKey NamedKey = static_cast<ImGuiKey>(Key);
			if (ImGui::IsMouseKey(NamedKey)) continue;
			if (!(ImGui::IsGamepadKey(NamedKey) ? bGamepad : bKeyboard)) continue;
			ImGuiKeyData& Data = IO.KeysData[Key - ImGuiKey_NamedKey_BEGIN];
			Data.Down = false;
			Data.AnalogValue = 0.f;
			Data.DownDuration = Data.DownDurationPrev = -1.f;
		}
		if (bKeyboard)
		{
			IO.KeyCtrl = IO.KeyShift = IO.KeyAlt = IO.KeySuper = false;
			IO.KeyMods = ImGuiMod_None;
			IO.InputQueueCharacters.resize(0);
			IO.InputQueueSurrogate = 0;
		}
		if (bMouse) IO.ClearInputMouse();
	}

	void UpdateFontAtlas(ImFontAtlas& Atlas)
	{
		const int FrameNumber = static_cast<int>(GFrameNumber & MAX_int32);
		if (!Atlas.Builder || Atlas.Builder->FrameCount < FrameNumber)
		{
			ImFontAtlasUpdateNewFrame(&Atlas, FrameNumber, true);
		}
	}
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
