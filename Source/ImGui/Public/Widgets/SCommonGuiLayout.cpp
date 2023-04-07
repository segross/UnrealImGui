#include "SCommonGuiLayout.h"

#include "ImGuiModuleManager.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SDPIScaler.h"
#include <Widgets/Layout/SScaleBox.h>

void SCommonGuiLayout::Construct(const FArguments& InArgs)
{
	ModuleManager = InArgs._ModuleManager;
	Content = InArgs._Content;
	
	if (ModuleManager)
	{
		auto& Settings = ModuleManager->GetSettings();
		SetDPIScale(Settings.GetDPIScaleInfo());
		if (!Settings.OnDPIScaleChangedDelegate.IsBoundToObject(this))
		{
			Settings.OnDPIScaleChangedDelegate.AddRaw(this, &SCommonGuiLayout::SetDPIScale);
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
			.DPIScale(TAttribute<float>(this, &SCommonGuiLayout::GetDPIScale))
			.Visibility(EVisibility::SelfHitTestInvisible)
			[
				SNew(SConstraintCanvas)
				+ SConstraintCanvas::Slot()
				.Anchors(FAnchors(0.f, 0.f, 1.f, 1.f))
				.AutoSize(true)
				.Offset(FMargin(1.f, 1.f, 0.f, 1.f))
				.Alignment(FVector2D::ZeroVector)
				[
					Content.ToSharedRef()
				]
			]
		]
	];

	SetVisibility(EVisibility::SelfHitTestInvisible);
}
