// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiContextProxy.h"
#include "ImGuiDemo.h"
#include "Utilities/WorldContextIndex.h"


// Manages ImGui context proxies.
class FImGuiContextManager
{
public:

	FImGuiContextManager() = default;

	FImGuiContextManager(const FImGuiContextManager&) = delete;
	FImGuiContextManager& operator=(const FImGuiContextManager&) = delete;

	FImGuiContextManager(FImGuiContextManager&&) = delete;
	FImGuiContextManager& operator=(FImGuiContextManager&&) = delete;

	// Get or create default ImGui context proxy. In editor this is the editor context proxy and in standalone game
	// context proxy for the only world and the same value as returned from GetWorldContextProxy.
	//
	// If proxy doesn't exist then it will be created and initialized.
	FImGuiContextProxy& GetDefaultContextProxy() { return FindOrAddContextData(Utilities::DEFAULT_CONTEXT_INDEX).ContextProxy; }

	// Get or create ImGui context proxy for given world.
	//
	// If proxy doesn't yet exist then it will be created and initialized. If proxy already exists then associated
	// world data will be updated.
	FImGuiContextProxy& GetWorldContextProxy(UWorld& World);

	// Get context proxy by index, or null if context with that index doesn't exist.
	FORCEINLINE FImGuiContextProxy* GetContextProxy(int32 ContextIndex)
	{
		FContextData* Data = Contexts.Find(ContextIndex);
		return Data ? &(Data->ContextProxy) : nullptr;
	}

	void Tick(float DeltaSeconds);

private:

	struct FContextData
	{
		TWeakObjectPtr<UWorld> World;
		FImGuiContextProxy ContextProxy;
	};

	FContextData& FindOrAddContextData(int32 Index);

	TMap<int32, FContextData> Contexts;

	FImGuiDemo ImGuiDemo;
};
