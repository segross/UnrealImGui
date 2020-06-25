// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <Engine/Engine.h>
#include <Engine/GameInstance.h>
#include <Engine/GameViewportClient.h>
#include <UObject/WeakObjectPtr.h>


// Utilities helping to get a World Context.

namespace Utilities
{
	template<typename T>
	FORCEINLINE const FWorldContext* GetWorldContext(const T* Obj);

	FORCEINLINE const FWorldContext* GetWorldContext(const UGameInstance& GameInstance)
	{
		return GameInstance.GetWorldContext();
	}

	template<typename T>
	FORCEINLINE const FWorldContext* GetWorldContext(const TWeakObjectPtr<T>& Obj)
	{
		return Obj.IsValid() ? GetWorldContext(*Obj.Get()) : nullptr;
	}

	FORCEINLINE const FWorldContext* GetWorldContext(const UGameViewportClient& GameViewportClient)
	{
		return GetWorldContext(GameViewportClient.GetGameInstance());
	}

	FORCEINLINE const FWorldContext* GetWorldContext(const UWorld& World)
	{
		return GetWorldContext(World.GetGameInstance());
	}

	template<typename T>
	FORCEINLINE const FWorldContext* GetWorldContext(const T* Obj)
	{
		return Obj ? GetWorldContext(*Obj) : nullptr;
	}

	const FWorldContext* GetWorldContextFromNetMode(ENetMode NetMode);
}
