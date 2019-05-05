// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include <Runtime/Launch/Resources/Version.h>

#define BELOW_ENGINE_VERSION(Major, Minor)  (ENGINE_MAJOR_VERSION < (Major) || (ENGINE_MAJOR_VERSION == (Major) && ENGINE_MINOR_VERSION < (Minor)))
#define FROM_ENGINE_VERSION(Major, Minor)   !BELOW_ENGINE_VERSION(Major, Minor)


// One place to define compatibility with older engine versions.


// Starting from version 4.17 Slate has an improved clipping API. Old version required to specify per-vertex clipping
// rectangle and unofficial GSlateScissorRect to correctly clip custom vertices made with FSlateDrawElement.
#define ENGINE_COMPATIBILITY_LEGACY_CLIPPING_API        BELOW_ENGINE_VERSION(4, 17)

// Starting from version 4.18 FPaths::GameSavedDir() has been superseded by FPaths::ProjectSavedDir().
#define ENGINE_COMPATIBILITY_LEGACY_SAVED_DIR           BELOW_ENGINE_VERSION(4, 18)

// Starting from version 4.18 we have support for dual key bindings.
#define ENGINE_COMPATIBILITY_SINGLE_KEY_BINDING         BELOW_ENGINE_VERSION(4, 18)

// Starting from version 4.18 FStringClassReference is replaced by FSoftClassPath. The new header contains a typedef
// that renames FStringClassReference to FSoftClassPath, so it is still possible tu use the old type name in code.
// The old header forwards to the new one but if used it outputs a warning, so we want to avoid it.
#define ENGINE_COMPATIBILITY_LEGACY_STRING_CLASS_REF    BELOW_ENGINE_VERSION(4, 18)

// Starting from version 4.18 engine has a world post actor tick event which if available, provides a good opportunity
// to call debug delegates after world actors are already updated.
#define ENGINE_COMPATIBILITY_WITH_WORLD_POST_ACTOR_TICK FROM_ENGINE_VERSION(4, 18)

