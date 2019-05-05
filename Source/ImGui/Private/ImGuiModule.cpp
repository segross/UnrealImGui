// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiModuleManager.h"

#include "ImGuiDelegatesContainer.h"
#include "ImGuiTextureHandle.h"
#include "TextureManager.h"
#include "Utilities/WorldContext.h"
#include "Utilities/WorldContextIndex.h"

#if WITH_EDITOR
#include "ImGuiImplementation.h"
#include "Editor/ImGuiEditor.h"
#endif

#include <IPluginManager.h>

#define IMGUI_REDIRECT_OBSOLETE_DELEGATES 1


#define LOCTEXT_NAMESPACE "FImGuiModule"


struct EDelegateCategory
{
	enum
	{
		// Default per-context draw events.
		Default,

		// Multi-context draw event defined in context manager.
		MultiContext
	};
};

static FImGuiModuleManager* ImGuiModuleManager = nullptr;

#if WITH_EDITOR
static FImGuiEditor* ImGuiEditor = nullptr;
#endif

#if IMGUI_WITH_OBSOLETE_DELEGATES

#if WITH_EDITOR
FImGuiDelegateHandle FImGuiModule::AddEditorImGuiDelegate(const FImGuiDelegate& Delegate)
{
#if IMGUI_REDIRECT_OBSOLETE_DELEGATES
	return { FImGuiDelegatesContainer::Get().OnWorldDebug(Utilities::EDITOR_CONTEXT_INDEX).Add(Delegate),
		EDelegateCategory::Default, Utilities::EDITOR_CONTEXT_INDEX };
#else
	checkf(ImGuiModuleManager, TEXT("Null pointer to internal module implementation. Is module available?"));

	return { ImGuiModuleManager->GetContextManager().GetEditorContextProxy().OnDraw().Add(Delegate),
		EDelegateCategory::Default, Utilities::EDITOR_CONTEXT_INDEX };
#endif // IMGUI_REDIRECT_OBSOLETE_DELEGATES
}
#endif

FImGuiDelegateHandle FImGuiModule::AddWorldImGuiDelegate(const FImGuiDelegate& Delegate)
{
#if IMGUI_REDIRECT_OBSOLETE_DELEGATES
	const int32 ContextIndex = Utilities::GetWorldContextIndex((UWorld*)GWorld);
	return { FImGuiDelegatesContainer::Get().OnWorldDebug(ContextIndex).Add(Delegate), EDelegateCategory::Default, ContextIndex };
#else
	checkf(ImGuiModuleManager, TEXT("Null pointer to internal module implementation. Is module available?"));

#if WITH_EDITOR
	checkf(GEngine, TEXT("Null GEngine. AddWorldImGuiDelegate should be only called with GEngine initialized."));

	const FWorldContext* WorldContext = Utilities::GetWorldContext(GEngine->GameViewport);
	if (!WorldContext)
	{
		WorldContext = Utilities::GetWorldContextFromNetMode(ENetMode::NM_DedicatedServer);
	}

	checkf(WorldContext, TEXT("Couldn't find current world. AddWorldImGuiDelegate should be only called from a valid world."));

	int32 Index;
	FImGuiContextProxy& Proxy = ImGuiModuleManager->GetContextManager().GetWorldContextProxy(*WorldContext->World(), Index);
#else
	const int32 Index = Utilities::STANDALONE_GAME_CONTEXT_INDEX;
	FImGuiContextProxy& Proxy = ImGuiModuleManager->GetContextManager().GetWorldContextProxy();
#endif

	return{ Proxy.OnDraw().Add(Delegate), EDelegateCategory::Default, Index };
#endif // IMGUI_REDIRECT_OBSOLETE_DELEGATES
}

FImGuiDelegateHandle FImGuiModule::AddMultiContextImGuiDelegate(const FImGuiDelegate& Delegate)
{
#if IMGUI_REDIRECT_OBSOLETE_DELEGATES
	return { FImGuiDelegatesContainer::Get().OnMultiContextDebug().Add(Delegate), EDelegateCategory::MultiContext };
#else
	checkf(ImGuiModuleManager, TEXT("Null pointer to internal module implementation. Is module available?"));

	return { ImGuiModuleManager->GetContextManager().OnDrawMultiContext().Add(Delegate), EDelegateCategory::MultiContext };
#endif
}

void FImGuiModule::RemoveImGuiDelegate(const FImGuiDelegateHandle& Handle)
{
#if IMGUI_REDIRECT_OBSOLETE_DELEGATES
	if (Handle.Category == EDelegateCategory::MultiContext)
	{
		FImGuiDelegatesContainer::Get().OnMultiContextDebug().Remove(Handle.Handle);
	}
	else
	{
		FImGuiDelegatesContainer::Get().OnWorldDebug(Handle.Index).Remove(Handle.Handle);
	}
#else
	if (ImGuiModuleManager)
	{
		if (Handle.Category == EDelegateCategory::MultiContext)
		{
			ImGuiModuleManager->GetContextManager().OnDrawMultiContext().Remove(Handle.Handle);
		}
		else if (auto* Proxy = ImGuiModuleManager->GetContextManager().GetContextProxy(Handle.Index))
		{
			Proxy->OnDraw().Remove(Handle.Handle);
		}
	}
#endif
}

#endif // IMGUI_WITH_OBSOLETE_DELEGATES

FImGuiTextureHandle FImGuiModule::FindTextureHandle(const FName& Name)
{
	const TextureIndex Index = ImGuiModuleManager->GetTextureManager().FindTextureIndex(Name);
	return (Index != INDEX_NONE) ? FImGuiTextureHandle{ Name, ImGuiInterops::ToImTextureID(Index) } : FImGuiTextureHandle{};
}

FImGuiTextureHandle FImGuiModule::RegisterTexture(const FName& Name, class UTexture2D* Texture, bool bMakeUnique)
{
	const TextureIndex Index = ImGuiModuleManager->GetTextureManager().CreateTextureResources(Name, Texture, bMakeUnique);
	return FImGuiTextureHandle{ Name, ImGuiInterops::ToImTextureID(Index) };
}

void FImGuiModule::ReleaseTexture(const FImGuiTextureHandle& Handle)
{
	if (Handle.IsValid())
	{
		ImGuiModuleManager->GetTextureManager().ReleaseTextureResources(ImGuiInterops::ToTextureIndex(Handle.GetTextureId()));
	}
}

void FImGuiModule::StartupModule()
{
	// Create managers that implements module logic.

	checkf(!ImGuiModuleManager, TEXT("Instance of the ImGui Module Manager already exists. Instance should be created only during module startup."));
	ImGuiModuleManager = new FImGuiModuleManager();

#if WITH_EDITOR
	checkf(!ImGuiEditor, TEXT("Instance of the ImGui Editor already exists. Instance should be created only during module startup."));
	ImGuiEditor = new FImGuiEditor();
#endif
}

void FImGuiModule::ShutdownModule()
{
	// In editor store data that we want to move to hot-reloaded module.

#if WITH_EDITOR
	static bool bMoveProperties = true;
	static FImGuiModuleProperties PropertiesToMove = ImGuiModuleManager->GetProperties();
#endif

	// Before we shutdown we need to delete managers that will do all the necessary cleanup.

#if WITH_EDITOR
	checkf(ImGuiEditor, TEXT("Null ImGui Editor. ImGui editor instance should be deleted during module shutdown."));
	delete ImGuiEditor;
	ImGuiEditor = nullptr;
#endif

	checkf(ImGuiModuleManager, TEXT("Null ImGui Module Manager. Module manager instance should be deleted during module shutdown."));
	delete ImGuiModuleManager;
	ImGuiModuleManager = nullptr;

#if WITH_EDITOR
	// When shutting down we leave the global ImGui context pointer and handle pointing to resources that are already
	// deleted. This can cause troubles after hot-reload when code in other modules calls ImGui interface functions
	// which are statically bound to the obsolete module. To keep ImGui code functional we can redirect context handle
	// to point to the new module.

	// When shutting down during hot-reloading, we might want to rewire handles used in statically bound functions
	// or move data to a new module.

	FModuleManager::Get().OnModulesChanged().AddLambda([this] (FName Name, EModuleChangeReason Reason)
	{
		if (Reason == EModuleChangeReason::ModuleLoaded && Name == "ImGui")
		{
			FImGuiModule& LoadedModule = FImGuiModule::Get();
			if (&LoadedModule != this)
			{
				// Statically bound functions will be still made to the obsolete module so we need to  
				ImGuiImplementation::SetImGuiContextHandle(LoadedModule.GetImGuiContextHandle());

				FImGuiDelegatesContainer::MoveContainer(LoadedModule.GetDelegatesContainer());

				if (bMoveProperties)
				{
					bMoveProperties = false;
					LoadedModule.SetProperties(PropertiesToMove);
				}
			}
		}
	});
#endif // WITH_EDITOR
}

#if WITH_EDITOR
void FImGuiModule::SetProperties(const FImGuiModuleProperties& Properties)
{
	ImGuiModuleManager->GetProperties() = Properties;
}

ImGuiContext** FImGuiModule::GetImGuiContextHandle()
{
	return ImGuiImplementation::GetImGuiContextHandle();
}

FImGuiDelegatesContainer& FImGuiModule::GetDelegatesContainer()
{
	return FImGuiDelegatesContainer::Get();
}
#endif

FImGuiModuleProperties& FImGuiModule::GetProperties()
{
	return ImGuiModuleManager->GetProperties();
}

const FImGuiModuleProperties& FImGuiModule::GetProperties() const
{
	return ImGuiModuleManager->GetProperties();
}

bool FImGuiModule::IsInputMode() const
{
	return ImGuiModuleManager && ImGuiModuleManager->GetProperties().IsInputEnabled();
}

void FImGuiModule::SetInputMode(bool bEnabled)
{
	if (ImGuiModuleManager)
	{
		ImGuiModuleManager->GetProperties().SetInputEnabled(bEnabled);
	}
}

void FImGuiModule::ToggleInputMode()
{
	if (ImGuiModuleManager)
	{
		ImGuiModuleManager->GetProperties().ToggleInput();
	}
}

bool FImGuiModule::IsShowingDemo() const
{
	return ImGuiModuleManager && ImGuiModuleManager->GetProperties().ShowDemo();
}

void FImGuiModule::SetShowDemo(bool bShow)
{
	if (ImGuiModuleManager)
	{
		ImGuiModuleManager->GetProperties().SetShowDemo(bShow);
	}
}

void FImGuiModule::ToggleShowDemo()
{
	if (ImGuiModuleManager)
	{
		ImGuiModuleManager->GetProperties().ToggleDemo();
	}
}


//----------------------------------------------------------------------------------------------------
// Runtime loader
//----------------------------------------------------------------------------------------------------

#if !WITH_EDITOR && RUNTIME_LOADER_ENABLED

class FImGuiModuleLoader
{
	FImGuiModuleLoader()
	{
		if (!Load())
		{
			FModuleManager::Get().OnModulesChanged().AddRaw(this, &FImGuiModuleLoader::LoadAndRelease);
		}
	}

	// For different engine versions.
	static FORCEINLINE bool IsValid(const TSharedPtr<IModuleInterface>& Ptr) { return Ptr.IsValid(); }
	static FORCEINLINE bool IsValid(const IModuleInterface* const Ptr) { return Ptr != nullptr; }

	bool Load()
	{
		return IsValid(FModuleManager::Get().LoadModule(ModuleName));
	}

	void LoadAndRelease(FName Name, EModuleChangeReason Reason)
	{
		// Avoid handling own load event.
		if (Name != ModuleName)
		{
			// Try loading until success and then release.
			if (Load())
			{
				FModuleManager::Get().OnModulesChanged().RemoveAll(this);
			}
		}
	}

	static FName ModuleName;

	static FImGuiModuleLoader Instance;
};

FName FImGuiModuleLoader::ModuleName = "ImGui";

// In monolithic builds this will start loading process.
FImGuiModuleLoader FImGuiModuleLoader::Instance;

#endif // !WITH_EDITOR && RUNTIME_LOADER_ENABLED


//----------------------------------------------------------------------------------------------------
// Partial implementations of other classes that needs access to ImGuiModuleManager
//----------------------------------------------------------------------------------------------------

bool FImGuiTextureHandle::HasValidEntry() const
{
	const TextureIndex Index = ImGuiInterops::ToTextureIndex(TextureId);
	return Index != INDEX_NONE && ImGuiModuleManager && ImGuiModuleManager->GetTextureManager().GetTextureName(Index) == Name;
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FImGuiModule, ImGui)
