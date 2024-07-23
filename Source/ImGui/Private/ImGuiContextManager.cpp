// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiContextManager.h"

#include "ImGuiDelegatesContainer.h"
#include "ImGuiImplementation.h"
#include "ImGuiModuleSettings.h"
#include "ImGuiModule.h"
#include "Utilities/WorldContext.h"
#include "Utilities/WorldContextIndex.h"

#include <imgui.h>


// TODO: Refactor ImGui Context Manager, to handle different types of worlds.

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

FImGuiContextManager::FImGuiContextManager(FImGuiModuleSettings& InSettings)
	: Settings(InSettings)
{
	Settings.OnDPIScaleChangedDelegate.AddRaw(this, &FImGuiContextManager::SetDPIScale);

	SetDPIScale(Settings.GetDPIScaleInfo());
	BuildFontAtlas();

	FWorldDelegates::OnWorldTickStart.AddRaw(this, &FImGuiContextManager::OnWorldTickStart);
#if ENGINE_COMPATIBILITY_WITH_WORLD_POST_ACTOR_TICK
	FWorldDelegates::OnWorldPostActorTick.AddRaw(this, &FImGuiContextManager::OnWorldPostActorTick);
#endif
}

FImGuiContextManager::~FImGuiContextManager()
{
	Settings.OnDPIScaleChangedDelegate.RemoveAll(this);

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
			ContextData.ContextProxy->Tick(DeltaSeconds);
		}
		else
		{
			// Clear to make sure that we don't store objects registered for world that is no longer valid.
			FImGuiDelegatesContainer::Get().OnWorldDebug(Pair.Key).Clear();
		}
	}

	// Once all context tick they should use new fonts and we can release the old resources. Extra countdown is added
	// wait for contexts that ticked outside of this function, before rebuilding fonts.
	if (FontResourcesReleaseCountdown > 0 && !--FontResourcesReleaseCountdown)
	{
		FontResourcesToRelease.Empty();
	}
}

#if ENGINE_COMPATIBILITY_LEGACY_WORLD_ACTOR_TICK
void FImGuiContextManager::OnWorldTickStart(ELevelTick TickType, float DeltaSeconds)
{
	OnWorldTickStart(GWorld, TickType, DeltaSeconds);
}
#endif

void FImGuiContextManager::OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	if (World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE
		|| World->WorldType == EWorldType::Editor))
	{
		FImGuiContextProxy& ContextProxy = GetWorldContextProxy(*World);

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
	if (World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE
		|| World->WorldType == EWorldType::Editor))
	{
		GetWorldContextProxy(*World).DrawDebug();
	}
}
#endif // ENGINE_COMPATIBILITY_WITH_WORLD_POST_ACTOR_TICK

#if WITH_EDITOR
FImGuiContextManager::FContextData& FImGuiContextManager::GetEditorContextData()
{
	FContextData* Data = Contexts.Find(Utilities::EDITOR_CONTEXT_INDEX);

	if (UNLIKELY(!Data))
	{
		Data = &Contexts.Emplace(Utilities::EDITOR_CONTEXT_INDEX, FContextData{ GetEditorContextName(), Utilities::EDITOR_CONTEXT_INDEX, FontAtlas, DPIScale, -1 });
		OnContextProxyCreated.Broadcast(Utilities::EDITOR_CONTEXT_INDEX, *Data->ContextProxy);
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
		Data = &Contexts.Emplace(Utilities::STANDALONE_GAME_CONTEXT_INDEX, FContextData{ GetWorldContextName(), Utilities::STANDALONE_GAME_CONTEXT_INDEX, FontAtlas, DPIScale });
		OnContextProxyCreated.Broadcast(Utilities::STANDALONE_GAME_CONTEXT_INDEX, *Data->ContextProxy);
	}

	return *Data;
}
#endif // !WITH_EDITOR

FImGuiContextManager::FContextData& FImGuiContextManager::GetWorldContextData(const UWorld& World, int32* OutIndex)
{
	using namespace Utilities;

#if WITH_EDITOR
	// Default to editor context for anything other than a game world.
	if (World.WorldType != EWorldType::Game && World.WorldType != EWorldType::PIE)
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
		Data = &Contexts.Emplace(Index, FContextData{ GetWorldContextName(World), Index, FontAtlas, DPIScale, WorldContext->PIEInstance });
		OnContextProxyCreated.Broadcast(Index, *Data->ContextProxy);
	}
	else
	{
		// Because we allow (for the sake of continuity) to map different PIE instances to the same index.
		Data->PIEInstance = WorldContext->PIEInstance;
	}
#else
	if (UNLIKELY(!Data))
	{
		Data = &Contexts.Emplace(Index, FContextData{ GetWorldContextName(World), Index, FontAtlas, DPIScale });
		OnContextProxyCreated.Broadcast(Index, *Data->ContextProxy);
	}
#endif

	if (OutIndex)
	{
		*OutIndex = Index;
	}
	return *Data;
}

void FImGuiContextManager::SetDPIScale(const FImGuiDPIScaleInfo& ScaleInfo)
{
	const float Scale = ScaleInfo.GetImGuiScale();
	if (DPIScale != Scale)
	{
		DPIScale = Scale;

		// Only rebuild font atlas if it is already built. Otherwise allow the other logic to pick a moment.
		if (FontAtlas.IsBuilt())
		{
			RebuildFontAtlas();
		}

		for (auto& Pair : Contexts)
		{
			if (Pair.Value.ContextProxy)
			{
				Pair.Value.ContextProxy->SetDPIScale(DPIScale);
			}
		}
	}
}

void FImGuiContextManager::BuildFontAtlas(const TMap<FName, TSharedPtr<ImFontConfig>>& CustomFontConfigs)
{
	if (!FontAtlas.IsBuilt())
	{
		ImFontConfig FontConfig = {};
		FontConfig.SizePixels = FMath::RoundFromZero(13.f * DPIScale);
		FontAtlas.AddFontDefault(&FontConfig);

		// Build custom fonts
		for (const TPair<FName, TSharedPtr<ImFontConfig>>& CustomFontPair : CustomFontConfigs)
		{
			FName CustomFontName = CustomFontPair.Key;
			TSharedPtr<ImFontConfig> CustomFontConfig = CustomFontPair.Value;

			// Set font name for debugging
			if (CustomFontConfig.IsValid())
			{
				strcpy_s(CustomFontConfig->Name, 40, TCHAR_TO_ANSI(*CustomFontName.ToString()));
			}
		
			FontAtlas.AddFont(CustomFontConfig.Get());
		}

		unsigned char* Pixels;
		int Width, Height, Bpp;
		FontAtlas.GetTexDataAsRGBA32(&Pixels, &Width, &Height, &Bpp);

		OnFontAtlasBuilt.Broadcast();
	}
}

void FImGuiContextManager::RebuildFontAtlas()
{
	if (FontAtlas.IsBuilt())
	{
		// Keep the old resources alive for a few frames to give all contexts a chance to bind to new ones.
		FontResourcesToRelease.Add(TUniquePtr<ImFontAtlas>(new ImFontAtlas()));
		Swap(*FontResourcesToRelease.Last(), FontAtlas);

		// Typically, one frame should be enough but since we allow for custom ticking, we need at least to frames to
		// wait for contexts that already ticked and will not do that before the end of the next tick of this manager.
		FontResourcesReleaseCountdown = 3;
	}

	BuildFontAtlas(FImGuiModule::Get().GetProperties().GetCustomFonts());
}
