// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiModuleManager.h"

#include "ImGuiInteroperability.h"
#include "Utilities/WorldContextIndex.h"

#include <imgui.h>


FImGuiModuleManager::FImGuiModuleManager()
{
	// Typically we will use viewport created events to add widget to new game viewports.
	ViewportCreatedHandle = UGameViewportClient::OnViewportCreated().AddRaw(this, &FImGuiModuleManager::OnViewportCreated);

	// Initialize resources and start ticking. Depending on loading phase, this may fail if Slate is not yet ready.
	Initialize();

	// We need to add widgets to active game viewports as they won't generate on-created events. This is especially
	// important during hot-reloading.
	if (bInitialized)
	{
		AddWidgetToAllViewports();
	}
}

FImGuiModuleManager::~FImGuiModuleManager()
{
	// We are no longer interested with adding widgets to viewports.
	if (ViewportCreatedHandle.IsValid())
	{
		UGameViewportClient::OnViewportCreated().Remove(ViewportCreatedHandle);
		ViewportCreatedHandle.Reset();
	}

	// Remove still active widgets (important during hot-reloading).
	for (auto& Widget : Widgets)
	{
		if (Widget.IsValid())
		{
			Widget.Pin()->Detach();
		}
	}

	// Deactivate this manager.
	Uninitialize();
}

void FImGuiModuleManager::Initialize()
{
	// We rely on Slate, so we can only continue if it is already initialized.
	if (!bInitialized && FSlateApplication::IsInitialized())
	{
		bInitialized = true;
		LoadTextures();
		RegisterTick();
	}
}

void FImGuiModuleManager::Uninitialize()
{
	if (bInitialized)
	{
		bInitialized = false;
		UnregisterTick();
	}
}

void FImGuiModuleManager::LoadTextures()
{
	checkf(FSlateApplication::IsInitialized(), TEXT("Slate should be initialized before we can create textures."));

	// Create an empty texture at index 0. We will use it for ImGui outputs with null texture id.
	TextureManager.CreatePlainTexture(FName{ "ImGuiModule_Plain" }, 2, 2, FColor::White);

	// Create a font atlas texture.
	ImFontAtlas* Fonts = ImGui::GetIO().Fonts;
	checkf(Fonts, TEXT("Invalid font atlas."));

	unsigned char* Pixels;
	int Width, Height, Bpp;
	Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height, &Bpp);

	TextureIndex FontsTexureIndex = TextureManager.CreateTexture(FName{ "ImGuiModule_FontAtlas" }, Width, Height, Bpp, Pixels, false);

	// Set font texture index in ImGui.
	Fonts->TexID = ImGuiInterops::ToImTextureID(FontsTexureIndex);
}

void FImGuiModuleManager::RegisterTick()
{
	checkf(FSlateApplication::IsInitialized(), TEXT("Slate should be initialized before we can register tick listener."));

	// We will tick on Slate Post-Tick events. They are quite convenient as they happen at the very end of the frame,
	// what helps to minimise tearing.
	if (!TickDelegateHandle.IsValid())
	{
		TickDelegateHandle = FSlateApplication::Get().OnPostTick().AddRaw(this, &FImGuiModuleManager::Tick);
	}
}

void FImGuiModuleManager::UnregisterTick()
{
	if (TickDelegateHandle.IsValid())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().OnPostTick().Remove(TickDelegateHandle);
		}
		TickDelegateHandle.Reset();
	}
}

bool FImGuiModuleManager::IsInUpdateThread()
{
	// We can get ticks from the Game thread and Slate loading thread. In both cases IsInGameThread() is true, so we
	// need to make additional test to filter out loading thread.
	return IsInGameThread() && !IsInSlateThread();
}

void FImGuiModuleManager::Tick(float DeltaSeconds)
{
	if (IsInUpdateThread())
	{
		// Update context manager to advance all ImGui contexts to the next frame.
		ContextManager.Tick(DeltaSeconds);

		// Inform that we finished updating ImGui, so other subsystems can react.
		PostImGuiUpdateEvent.Broadcast();
	}
}

void FImGuiModuleManager::OnViewportCreated()
{
	checkf(FSlateApplication::IsInitialized(), TEXT("We expect Slate to be initialized when game viewport is created."));

	// Make sure that all resources are initialized to handle configurations where this module is loaded before Slate.
	Initialize();

	// Create widget to viewport responsible for this event.
	AddWidgetToViewport(GEngine->GameViewport);
}

void FImGuiModuleManager::AddWidgetToViewport(UGameViewportClient* GameViewport)
{
	checkf(GameViewport, TEXT("Null game viewport."));
	checkf(FSlateApplication::IsInitialized(), TEXT("Slate should be initialized before we can add widget to game viewports."));

	// This makes sure that context for this world is created.
	auto& Proxy = ContextManager.GetWorldContextProxy(*GameViewport->GetWorld());

	TSharedPtr<SImGuiWidget> SharedWidget;
	SAssignNew(SharedWidget, SImGuiWidget).ModuleManager(this).GameViewport(GameViewport)
		.ContextIndex(Utilities::GetWorldContextIndex(GameViewport));

	if (TWeakPtr<SImGuiWidget>* Slot = Widgets.FindByPredicate([](auto& Widget) { return !Widget.IsValid(); }))
	{
		*Slot = SharedWidget;
	}
	else
	{
		Widgets.Emplace(SharedWidget);
	}
}

void FImGuiModuleManager::AddWidgetToAllViewports()
{
	checkf(FSlateApplication::IsInitialized(), TEXT("Slate should be initialized before we can add widget to game viewports."));

	if (GEngine)
	{
		// Loop as long as we have a valid viewport or until we detect a cycle.
		UGameViewportClient* GameViewport = GEngine->GameViewport;
		while (GameViewport)
		{
			AddWidgetToViewport(GameViewport);

			GameViewport = GEngine->GetNextPIEViewport(GameViewport);
			if (GameViewport == GEngine->GameViewport)
			{
				break;
			}
		}
	}
}
