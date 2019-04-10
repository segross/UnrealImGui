// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "Utilities/WorldContext.h"


// Utilities mapping worlds to indices that we use to identify ImGui contexts.
// Editor and standalone games have context index 0 while PIE worlds have indices starting from 1 for server and 2+ for
// clients.

namespace Utilities
{
	// Invalid context index for parameters that cannot be resolved to a valid world.
	static constexpr int32 INVALID_CONTEXT_INDEX = -1;

	// Standalone context index.
	static constexpr int32 STANDALONE_GAME_CONTEXT_INDEX = 0;

#if WITH_EDITOR

	// Editor context index.
	static constexpr int32 EDITOR_CONTEXT_INDEX = 0;

	FORCEINLINE int32 GetWorldContextIndex(const FWorldContext& WorldContext)
	{
		// In standalone game (WorldType = Game) we have only one context with index 0 (see GAME_CONTEXT_INDEX).

		// In editor, we keep 0 for editor and use PIEInstance to index worlds. In simulation or standalone single-PIE
		// sessions PIEInstance is 0, but since there is only one world we can change it without causing any conflicts.
		// In single-PIE with dedicated server or multi-PIE sessions worlds have PIEInstance starting from 1 for server
		// and 2+ for clients, what maps directly to our index.

		switch (WorldContext.WorldType)
		{
		case EWorldType::PIE:
			return FMath::Max(WorldContext.PIEInstance, 1);
		case EWorldType::Game:
			return STANDALONE_GAME_CONTEXT_INDEX;
		case EWorldType::Editor:
			return EDITOR_CONTEXT_INDEX;
		default:
			return INVALID_CONTEXT_INDEX;
		}
	}

	template<typename T>
	FORCEINLINE int32 GetWorldContextIndex(const T& Obj)
	{
		const FWorldContext* WorldContext = GetWorldContext(Obj);
		return WorldContext ? GetWorldContextIndex(*WorldContext) : INVALID_CONTEXT_INDEX;
	}

	FORCEINLINE int32 GetWorldContextIndex(const UWorld& World)
	{
		return (World.WorldType == EWorldType::Editor) ? EDITOR_CONTEXT_INDEX : GetWorldContextIndex(World.GetGameInstance());
	}

	FORCEINLINE int32 GetWorldContextIndex(const UWorld* World)
	{
		return World ? GetWorldContextIndex(*World) : INVALID_CONTEXT_INDEX;
	}

#else

	template<typename T>
	constexpr int32 GetWorldContextIndex(const T&)
	{
		// The only option is standalone game with one context.
		return STANDALONE_GAME_CONTEXT_INDEX;
	}

#endif // #if WITH_EDITOR
}
