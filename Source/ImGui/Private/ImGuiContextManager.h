// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiContextProxy.h"


// TODO: It might be useful to broadcast FContextProxyCreatedDelegate to users, to support similar cases to our ImGui
// demo, but we would need to remove from that interface internal classes.

// Delegate called when new context proxy is created.
// @param ContextIndex - Index for that world
// @param ContextProxy - Created context proxy
DECLARE_MULTICAST_DELEGATE_TwoParams(FContextProxyCreatedDelegate, int32, FImGuiContextProxy&);

// Manages ImGui context proxies.
class FImGuiContextManager
{
public:

	FImGuiContextManager();

	FImGuiContextManager(const FImGuiContextManager&) = delete;
	FImGuiContextManager& operator=(const FImGuiContextManager&) = delete;

	FImGuiContextManager(FImGuiContextManager&&) = delete;
	FImGuiContextManager& operator=(FImGuiContextManager&&) = delete;

	~FImGuiContextManager();

	ImFontAtlas& GetFontAtlas() { return FontAtlas; }
	const ImFontAtlas& GetFontAtlas() const { return FontAtlas; }


#if WITH_EDITOR
	// Get or create editor ImGui context proxy.
	FORCEINLINE FImGuiContextProxy& GetEditorContextProxy() { return GetEditorContextData().ContextProxy; }
#endif

#if !WITH_EDITOR
	// Get or create standalone game ImGui context proxy.
	FORCEINLINE FImGuiContextProxy& GetWorldContextProxy() { return GetStandaloneWorldContextData().ContextProxy; }
#endif

	// Get or create ImGui context proxy for given world.
	FORCEINLINE FImGuiContextProxy& GetWorldContextProxy(const UWorld& World) { return GetWorldContextData(World).ContextProxy; }

	// Get or create ImGui context proxy for given world. Additionally get context index for that proxy.
	FORCEINLINE FImGuiContextProxy& GetWorldContextProxy(const UWorld& World, int32& OutContextIndex) { return GetWorldContextData(World, &OutContextIndex).ContextProxy; }

	// Get context proxy by index, or null if context with that index doesn't exist.
	FORCEINLINE FImGuiContextProxy* GetContextProxy(int32 ContextIndex)
	{
		FContextData* Data = Contexts.Find(ContextIndex);
		return Data ? &(Data->ContextProxy) : nullptr;
	}

	// Delegate called for all contexts in manager, right after calling context specific draw event. Allows listeners
	// draw the same content to multiple contexts.
	FSimpleMulticastDelegate& OnDrawMultiContext() { return DrawMultiContextEvent; }

	// Delegate called when new context proxy is created.
	FContextProxyCreatedDelegate& OnContextProxyCreated() { return ContextProxyCreatedEvent; }

	void Tick(float DeltaSeconds);

private:

#if WITH_EDITOR

	struct FContextData
	{
		FContextData(const FString& ContextName, int32 ContextIndex, FSimpleMulticastDelegate& SharedDrawEvent, ImFontAtlas& FontAtlas, int32 InPIEInstance = -1)
			: PIEInstance(InPIEInstance)
			, ContextProxy(ContextName, ContextIndex, &SharedDrawEvent, &FontAtlas)
		{
		}

		FORCEINLINE bool CanTick() const { return PIEInstance < 0 || GEngine->GetWorldContextFromPIEInstance(PIEInstance); }

		int32 PIEInstance = -1;
		FImGuiContextProxy ContextProxy;
	};

#else

	struct FContextData
	{
		FContextData(const FString& ContextName, int32 ContextIndex, FSimpleMulticastDelegate& SharedDrawEvent, ImFontAtlas& FontAtlas)
			: ContextProxy(ContextName, ContextIndex, &SharedDrawEvent, &FontAtlas)
		{
		}

		FORCEINLINE bool CanTick() const { return true; }

		FImGuiContextProxy ContextProxy;
	};

#endif // WITH_EDITOR

	void OnWorldTickStart(ELevelTick TickType, float DeltaSeconds);

#if ENGINE_COMPATIBILITY_WITH_WORLD_POST_ACTOR_TICK
	void OnWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);
#endif

#if WITH_EDITOR
	FContextData& GetEditorContextData();
#endif

#if !WITH_EDITOR
	FContextData& GetStandaloneWorldContextData();
#endif

	FContextData& GetWorldContextData(const UWorld& World, int32* OutContextIndex = nullptr);

	TMap<int32, FContextData> Contexts;

	FSimpleMulticastDelegate DrawMultiContextEvent;

	FContextProxyCreatedDelegate ContextProxyCreatedEvent;

	ImFontAtlas FontAtlas;
};
