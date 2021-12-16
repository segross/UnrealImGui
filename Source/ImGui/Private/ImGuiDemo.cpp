// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiDemo.h"

#include "ImGuiModuleProperties.h"

#include <CoreGlobals.h>

// Demo copied (with minor modifications) from ImGui examples. See https://github.com/ocornut/imgui.
void FImGuiDemo::DrawControls(int32 ContextIndex)
{
    if (Properties.ShowDemo())
    {
        ImGui::ShowDemoWindow();
    }
}
