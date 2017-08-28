// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiContextManager.h"


FImGuiContextProxy& FImGuiContextManager::GetWorldContextProxy(UWorld& World)
{
	const int32 Index = Utilities::GetWorldContextIndex(World);
	checkf(Index != Utilities::INVALID_CONTEXT_INDEX, TEXT("Couldn't resolve context index for world %s: WorldType = %d"),
		*World.GetName(), World.WorldType);

#if WITH_EDITOR
	// Make sure that PIE worlds don't try to use editor context.
	checkf(!GEngine->IsEditor() || Index != Utilities::DEFAULT_CONTEXT_INDEX, TEXT("Index for world %s "
		"was resolved to the default context index %d, which in editor is reserved for editor context. PIE worlds "
		"should use values that start from 1. WorldType = %d, NetMode = %d"), *World.GetName(),
		Utilities::DEFAULT_CONTEXT_INDEX, World.WorldType, World.GetNetMode());
#endif // WITH_EDITOR

	FContextData& Data = FindOrAddContextData(Index);

	// Track worlds to make sure that different worlds don't try to use the same context in the same time.
	if (!Data.World.IsValid())
	{
		Data.World = &World;
	}
	else
	{
		checkf(Data.World == &World, TEXT("Two different worlds, %s and %s, resolved to the same world context index %d."),
			*Data.World->GetName(), *World.GetName(), Index);
	}

	return Data.ContextProxy;
}

void FImGuiContextManager::Tick(float DeltaSeconds)
{
	FContextData* Data = Contexts.Find(1);
	if (!Data || !Data->World.IsValid())
	{
		return;
	}

	for (auto& Entry : Contexts)
	{
		FImGuiContextProxy& ContextProxy = Entry.Value.ContextProxy;

		ContextProxy.SetAsCurrent();

		// Tick context proxy to end the old frame and starts a new one.
		ContextProxy.Tick(DeltaSeconds);
	}
}

FImGuiContextManager::FContextData& FImGuiContextManager::FindOrAddContextData(int32 Index)
{
	FContextData* Data = Contexts.Find(Index);

	if (!Data)
	{
		Data = &Contexts.Add(Index);
		if (!Data->ContextProxy.OnDraw().IsBoundToObject(&ImGuiDemo))
		{
			Data->ContextProxy.OnDraw().AddRaw(&ImGuiDemo, &FImGuiDemo::DrawControls);
		}
	}

	return *Data;
}
