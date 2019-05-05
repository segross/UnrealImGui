// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiContextManager.h"

#include "ImGuiDelegatesContainer.h"
#include "ImGuiImplementation.h"
#include "Utilities/ScopeGuards.h"
#include "Utilities/WorldContext.h"
#include "Utilities/WorldContextIndex.h"

#include <imgui.h>


namespace
{
#if WITH_EDITOR

	// Name for editor ImGui context.
	FORCEINLINE FString GetEditorContextName()
	{
		return TEXT("Editor");
	}

	// Name for world ImGui context.
	FORCEINLINE FString GetWorldContextName(const UWorld& World)
	{
		using namespace Utilities;

		const FWorldContext* WorldContext = GetWorldContext(World);
		switch (WorldContext->WorldType)
		{
		case EWorldType::PIE:
			return FString::Printf(TEXT("PIEContext%d"), GetWorldContextIndex(*WorldContext));
		case EWorldType::Game:
			return TEXT("Game");
		case EWorldType::Editor:
			return TEXT("Editor");
		default:
			return TEXT("Other");
		}
	}

#else

	FORCEINLINE FString GetWorldContextName()
	{
		return TEXT("Game");
	}

	FORCEINLINE FString GetWorldContextName(const UWorld&)
	{
		return TEXT("Game");
	}

#endif // WITH_EDITOR
}

FImGuiContextManager::FImGuiContextManager()
{
	unsigned char* Pixels;
	int Width, Height, Bpp;
	FontAtlas.GetTexDataAsRGBA32(&Pixels, &Width, &Height, &Bpp);

	FWorldDelegates::OnWorldTickStart.AddRaw(this, &FImGuiContextManager::OnWorldTickStart);
#if ENGINE_COMPATIBILITY_WITH_WORLD_POST_ACTOR_TICK
	FWorldDelegates::OnWorldPostActorTick.AddRaw(this, &FImGuiContextManager::OnWorldPostActorTick);
#endif
}

FImGuiContextManager::~FImGuiContextManager()
{
	// Order matters because contexts can be created during World Tick Start events.
	FWorldDelegates::OnWorldTickStart.RemoveAll(this);
#if ENGINE_COMPATIBILITY_WITH_WORLD_POST_ACTOR_TICK
	FWorldDelegates::OnWorldPostActorTick.RemoveAll(this);
#endif
}

void FImGuiContextManager::Tick(float DeltaSeconds)
{
	// In editor, worlds can get invalid. We could remove corresponding entries, but that would mean resetting ImGui
	// context every time when PIE session is restarted. Instead we freeze contexts until their worlds are re-created.

	for (auto& Pair : Contexts)
	{
		auto& ContextData = Pair.Value;
		if (ContextData.CanTick())
		{
			ContextData.ContextProxy.Tick(DeltaSeconds);
		}
		else
		{
			// Clear to make sure that we don't store objects registered for world that is no longer valid.
			FImGuiDelegatesContainer::Get().OnWorldDebug(Pair.Key).Clear();
		}
	}
}

void FImGuiContextManager::OnWorldTickStart(ELevelTick TickType, float DeltaSeconds)
{
	if (GWorld)
	{
		FImGuiContextProxy& ContextProxy = GetWorldContextProxy(*GWorld);

		// Set as current, so we have right context ready when updating world objects.
		ContextProxy.SetAsCurrent();

		ContextProxy.DrawEarlyDebug();
#if !ENGINE_COMPATIBILITY_WITH_WORLD_POST_ACTOR_TICK
		ContextProxy.DrawDebug();
#endif
	}
}

#if ENGINE_COMPATIBILITY_WITH_WORLD_POST_ACTOR_TICK
void FImGuiContextManager::OnWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	GetWorldContextProxy(*GWorld).DrawDebug();
}
#endif // ENGINE_COMPATIBILITY_WITH_WORLD_POST_ACTOR_TICK

#if WITH_EDITOR
FImGuiContextManager::FContextData& FImGuiContextManager::GetEditorContextData()
{
	FContextData* Data = Contexts.Find(Utilities::EDITOR_CONTEXT_INDEX);

	if (UNLIKELY(!Data))
	{
		Data = &Contexts.Emplace(Utilities::EDITOR_CONTEXT_INDEX, FContextData{ GetEditorContextName(), Utilities::EDITOR_CONTEXT_INDEX, DrawMultiContextEvent, FontAtlas, -1 });
		ContextProxyCreatedEvent.Broadcast(Utilities::EDITOR_CONTEXT_INDEX, Data->ContextProxy);
	}

	return *Data;
}
#endif // WITH_EDITOR

#if !WITH_EDITOR
FImGuiContextManager::FContextData& FImGuiContextManager::GetStandaloneWorldContextData()
{
	FContextData* Data = Contexts.Find(Utilities::STANDALONE_GAME_CONTEXT_INDEX);

	if (UNLIKELY(!Data))
	{
		Data = &Contexts.Emplace(Utilities::STANDALONE_GAME_CONTEXT_INDEX, FContextData{ GetWorldContextName(), Utilities::STANDALONE_GAME_CONTEXT_INDEX, DrawMultiContextEvent, FontAtlas });
		ContextProxyCreatedEvent.Broadcast(Utilities::STANDALONE_GAME_CONTEXT_INDEX, Data->ContextProxy);
	}

	return *Data;
}
#endif // !WITH_EDITOR

FImGuiContextManager::FContextData& FImGuiContextManager::GetWorldContextData(const UWorld& World, int32* OutIndex)
{
	using namespace Utilities;

#if WITH_EDITOR
	if (World.WorldType == EWorldType::Editor)
	{
		if (OutIndex)
		{
			*OutIndex = Utilities::EDITOR_CONTEXT_INDEX;
		}

		return GetEditorContextData();
	}
#endif

	const FWorldContext* WorldContext = GetWorldContext(World);
	const int32 Index = GetWorldContextIndex(*WorldContext);

	checkf(Index != Utilities::INVALID_CONTEXT_INDEX, TEXT("Couldn't find context index for world %s: WorldType = %d"),
		*World.GetName(), static_cast<int32>(World.WorldType));

#if WITH_EDITOR
	checkf(!GEngine->IsEditor() || Index != Utilities::EDITOR_CONTEXT_INDEX,
		TEXT("Context index %d is reserved for editor and cannot be used for world %s: WorldType = %d, NetMode = %d"),
		Index, *World.GetName(), static_cast<int32>(World.WorldType), static_cast<int32>(World.GetNetMode()));
#endif

	FContextData* Data = Contexts.Find(Index);

#if WITH_EDITOR
	if (UNLIKELY(!Data))
	{
		Data = &Contexts.Emplace(Index, FContextData{ GetWorldContextName(World), Index, DrawMultiContextEvent, FontAtlas, WorldContext->PIEInstance });
		ContextProxyCreatedEvent.Broadcast(Index, Data->ContextProxy);
	}
	else
	{
		// Because we allow (for the sake of continuity) to map different PIE instances to the same index.
		Data->PIEInstance = WorldContext->PIEInstance;
	}
#else
	if (UNLIKELY(!Data))
	{
		Data = &Contexts.Emplace(Index, FContextData{ GetWorldContextName(World), Index, DrawMultiContextEvent, FontAtlas });
		ContextProxyCreatedEvent.Broadcast(Index, Data->ContextProxy);
	}
#endif

	if (OutIndex)
	{
		*OutIndex = Index;
	}
	return *Data;
}
