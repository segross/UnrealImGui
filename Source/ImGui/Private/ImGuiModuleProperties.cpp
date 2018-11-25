// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiModuleProperties.h"


FImGuiModuleProperties& FImGuiModuleProperties::Get()
{
	static FImGuiModuleProperties Instance;
	return Instance;
}
