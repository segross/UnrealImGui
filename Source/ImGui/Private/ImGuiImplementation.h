// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <imgui.h>


// Gives access to selected ImGui implementation features.
namespace ImGuiImplementation
{
	// Get default context created by ImGui framework.
	ImGuiContext& GetDefaultContext();

	// Save current context settings.
	void SaveCurrentContextIniSettings(const char* Filename);
}
