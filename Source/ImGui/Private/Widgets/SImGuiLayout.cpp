// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "SImGuiLayout.h"
#include "SImGuiWidget.h"

#include "ImGuiModuleManager.h"
#include "ImGuiModuleSettings.h"

#include <SlateOptMacros.h>
#include <Widgets/Layout/SConstraintCanvas.h>
#include <Widgets/Layout/SDPIScaler.h>
#include <Widgets/Layout/SScaleBox.h>


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SImGuiLayout::Construct(const FArguments& InArgs)
{
	checkf(InArgs._GameViewport, TEXT("Null Game Viewport argument"));

	ModuleManager = InArgs._ModuleManager;
	GameViewport = InArgs._GameViewport;

	if (ModuleManager)
	{
		auto& Settings = ModuleManager->GetSettings();
		SetDPIScale(Settings.GetDPIScaleInfo());
		if (!Settings.OnDPIScaleChangedDelegate.IsBoundToObject(this))
		{
			Settings.OnDPIScaleChangedDelegate.AddRaw(this, &SImGuiLayout::SetDPIScale);
		}
	}

	ChildSlot
	[
		// Remove accumulated scale to manually control how we draw data. 
		SNew(SScaleBox)
		.IgnoreInheritedScale(true)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			// Apply custom scale if needed.
			SNew(SDPIScaler)
			.DPIScale(TAttribute<float>(this, &SImGuiLayout::GetDPIScale))
			.Visibility(EVisibility::SelfHitTestInvisible)
			[
				SNew(SConstraintCanvas)
				+ SConstraintCanvas::Slot()
				.Anchors(FAnchors(0.f, 0.f, 1.f, 1.f))
				.AutoSize(true)
				.Offset(FMargin(1.f, 1.f, 0.f, 1.f))
				.Alignment(FVector2D::ZeroVector)
				[
					SNew(SImGuiWidget)
					.ModuleManager(InArgs._ModuleManager)
					.GameViewport(InArgs._GameViewport)
					.ContextIndex(InArgs._ContextIndex)
#if !ENGINE_COMPATIBILITY_LEGACY_CLIPPING_API
					// To correctly clip borders. Using SScissorRectBox in older versions seems to be not necessary.
					.Clipping(EWidgetClipping::ClipToBounds)
#endif
				]
			]
		]
	];

	SetVisibility(EVisibility::SelfHitTestInvisible);
}

SImGuiLayout::~SImGuiLayout()
{
	if (ModuleManager)
	{
		ModuleManager->GetSettings().OnDPIScaleChangedDelegate.RemoveAll(this);
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SImGuiLayout::SetDPIScale(const FImGuiDPIScaleInfo& ScaleInfo)
{
	DPIScale = ScaleInfo.GetSlateScale();
}
