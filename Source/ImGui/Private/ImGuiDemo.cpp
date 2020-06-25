// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiDemo.h"

#include "ImGuiModuleProperties.h"

#include <CoreGlobals.h>


// Demo copied (with minor modifications) from ImGui examples. See https://github.com/ocornut/imgui.
void FImGuiDemo::DrawControls(int32 ContextIndex)
{
	if (Properties.ShowDemo())
	{
		const int32 ContextBit = ContextIndex < 0 ? 0 : 1 << ContextIndex;

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
			ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
			ImGui::Begin("Another Window");
			ImGui::Text("Hello");
			ImGui::End();
		}

		// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
		if (ShowDemoWindowMask & ContextBit)
		{
			// If more than one demo window is opened display warning about running ImGui examples in multiple contexts.

			// For everything, but the first windows in this frame we assume warning.
			bool bWarning = true;
			if (GFrameNumber > LastDemoWindowFrameNumber)
			{
				// If this is the first window in this frame, then we need to look at the last frame to see whether
				// there were more than one windows. Higher frame distance automatically means that there were not.
				bWarning = ((GFrameNumber - LastDemoWindowFrameNumber) == 1) && (DemoWindowCounter > 1);
	
				LastDemoWindowFrameNumber = GFrameNumber;
				DemoWindowCounter = 0;
			}

			DemoWindowCounter++;

			if (bWarning)
			{
				ImGui::Spacing();

				ImGui::PushStyleColor(ImGuiCol_Text, { 1.f, 1.f, 0.5f, 1.f });				
				ImGui::TextWrapped("Demo Window is opened in more than one context, some of the ImGui examples may not work correctly.");
				ImGui::PopStyleColor();

				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(
						"Some of the ImGui examples that use static variables may not work correctly\n"
						"when run concurrently in multiple contexts.\n"
						"If you have a problem with an example try to run it in one context only.");
				}
			}

			// Draw demo window.
			ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
			ImGui::ShowDemoWindow();
		}
	}
}
