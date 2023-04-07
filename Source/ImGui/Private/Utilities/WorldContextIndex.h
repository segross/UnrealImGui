// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImguiContextHandle.h"
#include "Utilities/WorldContext.h"


// Utilities mapping worlds to indices that we use to identify ImGui contexts.

namespace Utilities
{
	// Invalid context index for parameters that cannot be resolved to a valid world.
	static const FName INVALID_CONTEXT_INDEX = NAME_None;

	// Standalone context index.
	static const FName STANDALONE_GAME_CONTEXT_INDEX = {TEXT("STANDALONE_GAME_CONTEXT_INDEX")};

#if WITH_EDITOR

	// Editor context index. We are lacking flexibility here, so we might need to change it somehow.
	static const FName EDITOR_CONTEXT_INDEX = {TEXT("EDITOR_CONTEXT_INDEX")};

	FORCEINLINE FImguiContextHandle GetWorldContextIndex(const FWorldContext& WorldContext)
	{
		switch (WorldContext.WorldType)
		{
		case EWorldType::PIE:
			return FName(TEXT("PIE_INSTANCE") ,WorldContext.PIEInstance);
		case EWorldType::Game:
			return STANDALONE_GAME_CONTEXT_INDEX;
		case EWorldType::Editor:
			return EDITOR_CONTEXT_INDEX;
		default:
			return INVALID_CONTEXT_INDEX;
		}
	}

	template<typename T>
	FORCEINLINE FImguiContextHandle GetWorldContextIndex(const T& Obj)
	{
		const FWorldContext* WorldContext = GetWorldContext(Obj);
		return WorldContext ? GetWorldContextIndex(*WorldContext) : INVALID_CONTEXT_INDEX;
	}

	FORCEINLINE FImguiContextHandle GetWorldContextIndex(const UWorld& World)
	{
		return (World.WorldType == EWorldType::Editor) ? EDITOR_CONTEXT_INDEX : GetWorldContextIndex(World.GetGameInstance());
	}

	FORCEINLINE FImguiContextHandle GetWorldContextIndex(const UWorld* World)
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
