// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <Core.h>
#include <Engine.h>


// Utilities mapping worlds to indices that we use to identify ImGui contexts.
// Editor and standalone games have context index 0 while PIE worlds have indices starting from 1 for server and 2+ for
// clients.

namespace Utilities
{
	// Default context index for non PIE worlds.
	static constexpr int32 DEFAULT_CONTEXT_INDEX = 0;

	// Invalid context index for parameters that cannot be resolved to a valid world.
	static constexpr int32 INVALID_CONTEXT_INDEX = -1;

#if WITH_EDITOR

	template<typename T>
	FORCEINLINE int32 GetWorldContextIndex(const TWeakObjectPtr<T>& Obj)
	{
		return Obj.IsValid() ? GetWorldContextIndex(*Obj.Get()) : INVALID_CONTEXT_INDEX;
	}

	template<typename T>
	FORCEINLINE int32 GetWorldContextIndex(const T* Obj)
	{
		return Obj ? GetWorldContextIndex(*Obj) : INVALID_CONTEXT_INDEX;
	}

	FORCEINLINE int32 GetWorldContextIndex(const FWorldContext& WorldContext)
	{
		// In standalone game (WorldType = Game) we have only one context with index 0 (see DEFAULT_CONTEXT_INDEX).

		// In editor, we keep 0 for editor and use PIEInstance to index worlds. In simulation or standalone single-PIE
		// sessions PIEInstance is 0, but since there is only one world we can change it without causing any conflicts.
		// In single-PIE with dedicated server or multi-PIE sessions worlds have PIEInstance starting from 1 for server
		// and 2+ for clients, what maps directly to our index.

		return WorldContext.WorldType == EWorldType::PIE ? FMath::Max(WorldContext.PIEInstance, 1) : DEFAULT_CONTEXT_INDEX;
	}

	FORCEINLINE int32 GetWorldContextIndex(const UGameInstance& GameInstance)
	{
		return GetWorldContextIndex(GameInstance.GetWorldContext());
	}

	FORCEINLINE int32 GetWorldContextIndex(const UGameViewportClient& GameViewportClient)
	{
		return GetWorldContextIndex(GameViewportClient.GetGameInstance());
	}

	FORCEINLINE int32 GetWorldContextIndex(const UWorld& World)
	{
		return GetWorldContextIndex(World.GetGameInstance());
	}

#else

	template<typename T>
	constexpr int32 GetWorldContextIndex(const T&)
	{
		// The only option is standalone game with one context.
		return DEFAULT_CONTEXT_INDEX;
	}

#endif // #if WITH_EDITOR
}
