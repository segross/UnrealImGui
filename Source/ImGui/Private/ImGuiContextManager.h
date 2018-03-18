// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiContextProxy.h"
#include "ImGuiDemo.h"


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

	void Tick(float DeltaSeconds);

private:

#if WITH_EDITOR

	struct FContextData
	{
		FContextData(const FString& ContextName, int32 ContextIndex, FSimpleMulticastDelegate& SharedDrawEvent, FImGuiDemo& Demo, int32 InPIEInstance = -1)
			: PIEInstance(InPIEInstance)
			, ContextProxy(ContextName, &SharedDrawEvent)
		{
			ContextProxy.OnDraw().AddLambda([&Demo, ContextIndex]() { Demo.DrawControls(ContextIndex); });
		}

		FORCEINLINE bool CanTick() const { return PIEInstance < 0 || GEngine->GetWorldContextFromPIEInstance(PIEInstance); }

		int32 PIEInstance = -1;
		FImGuiContextProxy ContextProxy;
	};

#else

	struct FContextData
	{
		FContextData(const FString& ContextName, int32 ContextIndex, FSimpleMulticastDelegate& SharedDrawEvent, FImGuiDemo& Demo)
			: ContextProxy(ContextName, &SharedDrawEvent)
		{
			ContextProxy.OnDraw().AddLambda([&Demo, ContextIndex]() { Demo.DrawControls(ContextIndex); });
		}

		FORCEINLINE bool CanTick() const { return true; }

		FImGuiContextProxy ContextProxy;
	};

#endif // WITH_EDITOR

#if WITH_EDITOR
	void OnWorldTickStart(ELevelTick TickType, float DeltaSeconds);
#endif

#if WITH_EDITOR
	FContextData& GetEditorContextData();
#endif

#if !WITH_EDITOR
	FContextData& GetStandaloneWorldContextData();
#endif

	FContextData& GetWorldContextData(const UWorld& World, int32* OutContextIndex = nullptr);

	TMap<int32, FContextData> Contexts;

	FImGuiDemo ImGuiDemo;

	FSimpleMulticastDelegate DrawMultiContextEvent;
};
