// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

// Module-wide macros
#include "VersionCompatibility.h"
#include "ImGuiModuleDebug.h"

// Module
#include "ImGuiModule.h"

// Engine
#include <Core.h>
#include <Engine.h>

// For backward compatibility we will use FStringClassReference which in newer engine versions is a typedef for
// FSoftClassPath. Include right soft class reference header to avoid warnings in newer engine version.
#if ENGINE_COMPATIBILITY_LEGACY_STRING_CLASS_REF
#include <StringClassReference.h>
#else
#include <UObject/SoftObjectPath.h>
#endif

// You should place include statements to your module's private header files here.  You only need to
// add includes for headers that are used in most of your module's source files though.
