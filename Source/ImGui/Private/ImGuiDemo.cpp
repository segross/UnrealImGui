// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiDemo.h"
#include "ImGuiModuleManager.h"
#include "Utilities/ScopeGuards.h"


namespace CVars
{
	TAutoConsoleVariable<int> ShowDemo(TEXT("ImGui.ShowDemo"), 0,
		TEXT("Show ImGui demo.\n")
		TEXT("0: disabled (default)\n")
		TEXT("1: enabled."),
		ECVF_Default);
}

// Demo copied (with minor modifications) from ImGui examples. See https://github.com/ocornut/imgui.
void FImGuiDemo::DrawControls()
{
	if (CVars::ShowDemo.GetValueOnGameThread() > 0)
	{
		// TODO: This should be part of a public interface.
		extern int32 CurrentContextIndex;
		const int32 ContextBit = CurrentContextIndex < 0 ? 0 : 1 << CurrentContextIndex;

		// 1. Show a simple window
		// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
		{
			static float f = 0.0f;
			ImGui::Text("Hello, world!");
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
			ImGui::ColorEdit3("clear color", (float*)&ClearColor);

			if (ContextBit)
			{
				if (ImGui::Button("Demo Window")) ShowDemoWindowMask ^= ContextBit;
				if (ImGui::Button("Another Window")) ShowAnotherWindowMask ^= ContextBit;
			}
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		}

		// 2. Show another simple window, this time using an explicit Begin/End pair
		if (ShowAnotherWindowMask & ContextBit)
		{
			ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
			ImGui::Begin("Another Window");
			ImGui::Text("Hello");
			ImGui::End();
		}

		// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
		if (ShowDemoWindowMask & ContextBit)
		{
			// Display warning about running ImGui examples in multiple contexts.
			if (ShowDemoWindowMask != ContextBit)
			{
				ImGui::Spacing();

				ImGui::PushStyleColor(ImGuiCol_Text, { 1.f, 1.f, 0.5f, 1.f });				
				ImGui::TextWrapped("Demo Window is opend in more than one context, some of the ImGui examples may not work correctly.");
				ImGui::PopStyleColor();

				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(
						"Some of the ImGui examples that use static variables may not work correctly\n"
						"when run concurrently in multiple contexts.\n"
						"If you have a problem with an example try to run it in one context only.");
				}
			}
			ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
			ImGui::ShowDemoWindow();
		}
	}
}
