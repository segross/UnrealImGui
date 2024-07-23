// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiContextManager.h"
#include "ImGuiDemo.h"
#include "ImGuiModuleCommands.h"
#include "ImGuiModuleProperties.h"
#include "ImGuiModuleSettings.h"
#include "TextureManager.h"
#include "Widgets/SImGuiLayout.h"


// Central manager that implements module logic. It initializes and controls remaining module components.
class FImGuiModuleManager
{
	// Allow module to control life-cycle of this class.
	friend class FImGuiModule;

public:

	// Get interface to module settings.
	FImGuiModuleSettings& GetSettings() { return Settings; }

	// Get interface to module state properties. 
	FImGuiModuleProperties& GetProperties() { return Properties; }

	// Get ImGui contexts manager.
	FImGuiContextManager& GetContextManager() { return ContextManager; }

	// Get texture resources manager.
	FTextureManager& GetTextureManager() { return TextureManager; }

	// Event called right after ImGui is updated, to give other subsystems chance to react.
	FSimpleMulticastDelegate& OnPostImGuiUpdate() { return PostImGuiUpdateEvent; }

	void RebuildFontAtlas();

private:

	FImGuiModuleManager();
	~FImGuiModuleManager();

	FImGuiModuleManager(const FImGuiModuleManager&) = delete;
	FImGuiModuleManager& operator=(const FImGuiModuleManager&) = delete;

	FImGuiModuleManager(FImGuiModuleManager&&) = delete;
	FImGuiModuleManager& operator=(FImGuiModuleManager&&) = delete;

	void LoadTextures();
	void BuildFontAtlasTexture();

	bool IsTickRegistered() { return TickDelegateHandle.IsValid(); }
	void RegisterTick();
	void UnregisterTick();

	void CreateTickInitializer();
	void ReleaseTickInitializer();

	void Tick(float DeltaSeconds);

	void OnViewportCreated();

	void AddWidgetToViewport(UGameViewportClient* GameViewport);
	void AddWidgetsToActiveViewports();

	void OnContextProxyCreated(int32 ContextIndex, FImGuiContextProxy& ContextProxy);

	// Event that we call after ImGui is updated.
	FSimpleMulticastDelegate PostImGuiUpdateEvent;

	// Collection of module state properties.
	FImGuiModuleProperties Properties;

	// Tying module console commands to life-cycle of this manager and module.
	FImGuiModuleCommands Commands;

	// ImGui settings proxy (valid in every loading stage).
	FImGuiModuleSettings Settings;

	// Widget that we add to all created contexts to draw ImGui demo. 
	FImGuiDemo ImGuiDemo;

	// Manager for ImGui contexts.
	FImGuiContextManager ContextManager;

	// Manager for textures resources.
	FTextureManager TextureManager;

	// Slate widgets that we created.
	TArray<TWeakPtr<SImGuiLayout>> Widgets;

	FDelegateHandle TickInitializerHandle;
	FDelegateHandle TickDelegateHandle;
	FDelegateHandle ViewportCreatedHandle;

	bool bTexturesLoaded = false;
};
