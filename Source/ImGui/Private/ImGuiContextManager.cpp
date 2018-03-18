// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiContextManager.h"

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
#if WITH_EDITOR
	FWorldDelegates::OnWorldTickStart.AddRaw(this, &FImGuiContextManager::OnWorldTickStart);
#endif
}

FImGuiContextManager::~FImGuiContextManager()
{
#if WITH_EDITOR
	// Order matters because contexts can be created during World Tick Start events.
	FWorldDelegates::OnWorldTickStart.RemoveAll(this);
#endif
	Contexts.Empty();
	ImGui::Shutdown();
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
	}
}

#if WITH_EDITOR
void FImGuiContextManager::OnWorldTickStart(ELevelTick TickType, float DeltaSeconds)
{
	if (GWorld)
	{
		GetWorldContextProxy(*GWorld).SetAsCurrent();
	}
}
#endif // WITH_EDITOR

#if WITH_EDITOR
FImGuiContextManager::FContextData& FImGuiContextManager::GetEditorContextData()
{
	FContextData* Data = Contexts.Find(Utilities::EDITOR_CONTEXT_INDEX);

	if (UNLIKELY(!Data))
	{
		Data = &Contexts.Emplace(Utilities::EDITOR_CONTEXT_INDEX, FContextData{ GetEditorContextName(), Utilities::EDITOR_CONTEXT_INDEX, DrawMultiContextEvent, ImGuiDemo, -1 });
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
		Data = &Contexts.Emplace(Utilities::STANDALONE_GAME_CONTEXT_INDEX, FContextData{ GetWorldContextName(), Utilities::STANDALONE_GAME_CONTEXT_INDEX, DrawMultiContextEvent, ImGuiDemo });
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
		Data = &Contexts.Emplace(Index, FContextData{ GetWorldContextName(World), Index, DrawMultiContextEvent, ImGuiDemo, WorldContext->PIEInstance });
	}
	else
	{
		// Because we allow (for the sake of continuity) to map different PIE instances to the same index.
		Data->PIEInstance = WorldContext->PIEInstance;
	}
#else
	if (UNLIKELY(!Data))
	{
		Data = &Contexts.Emplace(Index, FContextData{ GetWorldContextName(World), Index, DrawMultiContextEvent, ImGuiDemo });
	}
#endif

	if (OutIndex)
	{
		*OutIndex = Index;
	}
	return *Data;
}
