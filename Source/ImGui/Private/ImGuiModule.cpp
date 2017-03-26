// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiModuleManager.h"

#include <IPluginManager.h>


#define LOCTEXT_NAMESPACE "FImGuiModule"


static FImGuiModuleManager* ModuleManager = nullptr;

void FImGuiModule::StartupModule()
{
	checkf(!ModuleManager, TEXT("Instance of Module Manager already exists. Instance should be created only during module startup."));

	// Create module manager that implements modules logic.
	ModuleManager = new FImGuiModuleManager();
}

void FImGuiModule::ShutdownModule()
{
	checkf(ModuleManager, TEXT("Null Module Manager. Manager instance should be deleted during module shutdown."));
	
	// Before we shutdown we need to delete manager that will do all necessary cleanup.
	delete ModuleManager;
	ModuleManager = nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FImGuiModule, ImGui)
