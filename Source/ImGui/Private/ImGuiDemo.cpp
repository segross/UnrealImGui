// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiDemo.h"
#include "ImGuiModuleManager.h"


namespace CVars
{
	TAutoConsoleVariable<int> ShowDemo(TEXT("ImGui.ShowDemo"), 0,
		TEXT("Show ImGui demo.\n")
		TEXT("0: disabled (default)\n")
		TEXT("1: enabled."),
		ECVF_Default);
}

// Demo copied from ImGui examples. See https://github.com/ocornut/imgui.
void FImGuiDemo::DrawControls()
{
	if (CVars::ShowDemo.GetValueOnGameThread() > 0)
	{
		// 1. Show a simple window
		// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
		{
			static float f = 0.0f;
			ImGui::Text("Hello, world!");
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
			ImGui::ColorEdit3("clear color", (float*)&ClearColor);
			if (ImGui::Button("Test Window")) bDemoShowTestWindow = !bDemoShowTestWindow;
			if (ImGui::Button("Another Window")) bDemoShowAnotherTestWindow = !bDemoShowAnotherTestWindow;
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		}

		// 2. Show another simple window, this time using an explicit Begin/End pair
		if (bDemoShowAnotherTestWindow)
		{
			ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
			ImGui::Begin("Another Window", &bDemoShowAnotherTestWindow);
			ImGui::Text("Hello");
			ImGui::End();
		}

		// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
		if (bDemoShowTestWindow)
		{
			ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
			ImGui::ShowTestWindow(&bDemoShowTestWindow);
		}
	}
}
