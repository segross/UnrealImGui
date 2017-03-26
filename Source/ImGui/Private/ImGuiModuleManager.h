// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiContextProxy.h"
#include "ImGuiDemo.h"
#include "SImGuiWidget.h"
#include "TextureManager.h"


// Central manager that implements module logic. It initializes and controls remaining module components.
class FImGuiModuleManager
{
	// Allow module to control life-cycle of this class.
	friend class FImGuiModule;

public:

	// Get ImGui context proxy.
	FImGuiContextProxy& GetContextProxy() { return ContextProxy; }

	// Get texture resources manager.
	FTextureManager& GetTextureManager() { return TextureManager; }

private:

	FImGuiModuleManager();
	~FImGuiModuleManager();

	FImGuiModuleManager(const FImGuiModuleManager&) = delete;
	FImGuiModuleManager& operator=(const FImGuiModuleManager&) = delete;

	FImGuiModuleManager(FImGuiModuleManager&&) = delete;
	FImGuiModuleManager& operator=(FImGuiModuleManager&&) = delete;

	void Initialize();
	void Uninitialize();

	void LoadTextures();

	void RegisterTick();
	void UnregisterTick();

	bool IsInUpdateThread();

	void Tick(float DeltaSeconds);

	void OnViewportCreated();

	void AddWidgetToViewport(UGameViewportClient* GameViewport);
	void AddWidgetToAllViewports();

	// Proxy controlling ImGui context.
	FImGuiContextProxy ContextProxy;

	// ImWidget that draws ImGui demo.
	FImGuiDemo ImGuiDemo;

	// Manager for textures resources.
	FTextureManager TextureManager;

	// Slate widget that we attach to created game viewports (widget without per-viewport state can be attached to
	// multiple viewports).
	TSharedPtr<SImGuiWidget> ViewportWidget;

	FDelegateHandle TickDelegateHandle;
	FDelegateHandle ViewportCreatedHandle;

	bool bInitialized = false;
};
