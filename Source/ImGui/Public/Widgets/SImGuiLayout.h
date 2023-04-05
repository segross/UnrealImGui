// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <UObject/WeakObjectPtr.h>
#include <Widgets/DeclarativeSyntaxSupport.h>
#include <Widgets/SCompoundWidget.h>


class FImGuiModuleManager;
class UGameViewportClient;
struct FImGuiDPIScaleInfo;

// Layout preset for ImGui Widget.
class SImGuiLayout : public SCompoundWidget
{
	typedef SCompoundWidget Super;

public:

	SLATE_BEGIN_ARGS(SImGuiLayout)
	{}
	SLATE_ARGUMENT(FImGuiModuleManager*, ModuleManager)
	SLATE_ARGUMENT(UGameViewportClient*, GameViewport)
	SLATE_ARGUMENT(int32, ContextIndex)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	~SImGuiLayout();

	const TWeakObjectPtr<UGameViewportClient>& GetGameViewport() const { return GameViewport; }

private:

	float GetDPIScale() const { return DPIScale; }
	void SetDPIScale(const FImGuiDPIScaleInfo& ScaleInfo);

	FImGuiModuleManager* ModuleManager = nullptr;
	TWeakObjectPtr<UGameViewportClient> GameViewport;

	float DPIScale = 1.f;
};
