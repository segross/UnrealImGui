// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiModuleManager.h"

#include "ImGuiInteroperability.h"
#include "Utilities/WorldContextIndex.h"

#include <ModuleManager.h>

#include <imgui.h>


FImGuiModuleManager::FImGuiModuleManager()
	: ModuleCommands(*this)
	, ImGuiDemo(*this)
{
	// Register in context manager to get information whenever a new context proxy is created.
	ContextManager.OnContextProxyCreated().AddRaw(this, &FImGuiModuleManager::OnContextProxyCreated);

	// Typically we will use viewport created events to add widget to new game viewports.
	ViewportCreatedHandle = UGameViewportClient::OnViewportCreated().AddRaw(this, &FImGuiModuleManager::OnViewportCreated);

	// Try to register tick delegate (it may fail if Slate application isn't yet ready).
	RegisterTick();

	// If we failed to register, create an initializer that will do it later.
	if (!IsTickRegistered())
	{
		CreateTickInitializer();
	}

	// We need to add widgets to active game viewports as they won't generate on-created events. This is especially
	// important during hot-reloading.
	AddWidgetsToActiveViewports();
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
	ReleaseTickInitializer();
	UnregisterTick();
}

void FImGuiModuleManager::LoadTextures()
{
	checkf(FSlateApplication::IsInitialized(), TEXT("Slate should be initialized before we can create textures."));

	if (!bTexturesLoaded)
	{
		bTexturesLoaded = true;

		TextureManager.InitializeErrorTexture(FColor::Magenta);

		// Create an empty texture at index 0. We will use it for ImGui outputs with null texture id.
		TextureManager.CreatePlainTexture(FName{ "ImGuiModule_Plain" }, 2, 2, FColor::White);

		// Create a font atlas texture.
		ImFontAtlas& Fonts = ContextManager.GetFontAtlas();

		unsigned char* Pixels;
		int Width, Height, Bpp;
		Fonts.GetTexDataAsRGBA32(&Pixels, &Width, &Height, &Bpp);

		TextureIndex FontsTexureIndex = TextureManager.CreateTexture(FName{ "ImGuiModule_FontAtlas" }, Width, Height, Bpp, Pixels);

		// Set font texture index in ImGui.
		Fonts.TexID = ImGuiInterops::ToImTextureID(FontsTexureIndex);
	}
}

void FImGuiModuleManager::RegisterTick()
{
	// Slate Post-Tick is a good moment to end and advance ImGui frame as it minimises a tearing.
	if (!TickDelegateHandle.IsValid() && FSlateApplication::IsInitialized())
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

void FImGuiModuleManager::CreateTickInitializer()
{
	if (!TickInitializerHandle.IsValid())
	{
		// Try to register tick delegate until we finally succeed.

		TickInitializerHandle = FModuleManager::Get().OnModulesChanged().AddLambda([this](FName Name, EModuleChangeReason Reason)
		{
			if (Reason == EModuleChangeReason::ModuleLoaded)
			{
				RegisterTick();
			}

			if (IsTickRegistered())
			{
				ReleaseTickInitializer();
			}
		});
	}
}

void FImGuiModuleManager::ReleaseTickInitializer()
{
	if (TickInitializerHandle.IsValid())
	{
		FModuleManager::Get().OnModulesChanged().Remove(TickInitializerHandle);
		TickInitializerHandle.Reset();
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

	// Create widget to viewport responsible for this event.
	AddWidgetToViewport(GEngine->GameViewport);
}

void FImGuiModuleManager::AddWidgetToViewport(UGameViewportClient* GameViewport)
{
	checkf(GameViewport, TEXT("Null game viewport."));
	checkf(FSlateApplication::IsInitialized(), TEXT("Slate should be initialized before we can add widget to game viewports."));

	// Make sure that we have a context for this viewport's world and get its index.
	int32 ContextIndex;
	auto& ContextProxy = ContextManager.GetWorldContextProxy(*GameViewport->GetWorld(), ContextIndex);

	// Make sure that textures are loaded before the first Slate widget is created.
	LoadTextures();

	// Create and initialize the widget.
	TSharedPtr<SImGuiWidget> SharedWidget;
	SAssignNew(SharedWidget, SImGuiWidget).ModuleManager(this).GameViewport(GameViewport).ContextIndex(ContextIndex);

	// We transfer widget ownerships to viewports but we keep weak references in case we need to manually detach active
	// widgets during module shutdown (important during hot-reloading).
	if (TWeakPtr<SImGuiWidget>* Slot = Widgets.FindByPredicate([](auto& Widget) { return !Widget.IsValid(); }))
	{
		*Slot = SharedWidget;
	}
	else
	{
		Widgets.Emplace(SharedWidget);
	}
}

void FImGuiModuleManager::AddWidgetsToActiveViewports()
{
	if (FSlateApplication::IsInitialized() && GEngine)
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

void FImGuiModuleManager::OnContextProxyCreated(int32 ContextIndex, FImGuiContextProxy& ContextProxy)
{
	ContextProxy.OnDraw().AddLambda([this, ContextIndex]() { ImGuiDemo.DrawControls(ContextIndex); });
}
