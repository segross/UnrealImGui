// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiModuleManager.h"

#include "ImGuiInteroperability.h"

#include <imgui.h>


FImGuiModuleManager::FImGuiModuleManager()
{
	// Bind ImGui demo to proxy, so it can draw controls in its context.
	ContextProxy.OnDraw().AddRaw(&ImGuiDemo, &FImGuiDemo::DrawControls);

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
	TextureManager.CreatePlainTexture(FName{ "ImGuiModule_Null" }, 2, 2, FColor::White);

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
		// Update context proxy to advance to next frame.
		ContextProxy.Tick(DeltaSeconds, ViewportWidget.IsValid() ? &ViewportWidget->GetInputState() : nullptr);

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

	if (!ViewportWidget.IsValid())
	{
		SAssignNew(ViewportWidget, SImGuiWidget).ModuleManager(this);
		checkf(ViewportWidget.IsValid(), TEXT("Failed to create SImGuiWidget."));
	}

	// High enough z-order guarantees that ImGui output is rendered on top of the game UI.
	constexpr int32 IMGUI_WIDGET_Z_ORDER = 10000;

	GameViewport->AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(ViewportWidget), IMGUI_WIDGET_Z_ORDER);
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
