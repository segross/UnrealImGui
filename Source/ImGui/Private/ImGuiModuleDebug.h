// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <Logging/LogMacros.h>


// Module-wide debug symbols and loggers.


// If enabled, it activates debug code and console variables that in normal usage are hidden.
#define IMGUI_MODULE_DEVELOPER 0


// Input Handler logger (used also in non-developer mode to raise problems with handler extensions).
DECLARE_LOG_CATEGORY_EXTERN(LogImGuiInputHandler, Warning, All);
