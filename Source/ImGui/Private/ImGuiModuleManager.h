// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiContextManager.h"
#include "ImGuiModuleCommands.h"
#include "SImGuiWidget.h"
#include "TextureManager.h"


// Central manager that implements module logic. It initializes and controls remaining module components.
class FImGuiModuleManager
{
	// Allow module to control life-cycle of this class.
	friend class FImGuiModule;

public:

	// Get ImGui contexts manager.
	FImGuiContextManager& GetContextManager() { return ContextManager; }

	// Get texture resources manager.
	FTextureManager& GetTextureManager() { return TextureManager; }

	// Event called right after ImGui is updated, to give other subsystems chance to react.
	FSimpleMulticastDelegate& OnPostImGuiUpdate() { return PostImGuiUpdateEvent; }

private:

	FImGuiModuleManager();
	~FImGuiModuleManager();

	FImGuiModuleManager(const FImGuiModuleManager&) = delete;
	FImGuiModuleManager& operator=(const FImGuiModuleManager&) = delete;

	FImGuiModuleManager(FImGuiModuleManager&&) = delete;
	FImGuiModuleManager& operator=(FImGuiModuleManager&&) = delete;

	void LoadTextures();

	bool IsTickRegistered() { return TickDelegateHandle.IsValid(); }
	void RegisterTick();
	void UnregisterTick();

	void CreateTickInitializer();
	void ReleaseTickInitializer();

	bool IsInUpdateThread();

	void Tick(float DeltaSeconds);

	void OnViewportCreated();

	void AddWidgetToViewport(UGameViewportClient* GameViewport);
	void AddWidgetsToActiveViewports();

	// Event that we call after ImGui is updated.
	FSimpleMulticastDelegate PostImGuiUpdateEvent;

	// Tying module console commands to life-cycle of this manager and module.
	FImGuiModuleCommands Commands;

	// Manager for ImGui contexts.
	FImGuiContextManager ContextManager;

	// Manager for textures resources.
	FTextureManager TextureManager;

	// Slate widgets that we created.
	TArray<TWeakPtr<SImGuiWidget>> Widgets;

	FDelegateHandle TickInitializerHandle;
	FDelegateHandle TickDelegateHandle;
	FDelegateHandle ViewportCreatedHandle;

	bool bTexturesLoaded = false;
};
