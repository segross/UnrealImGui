// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <Widgets/SLeafWidget.h>

class FImGuiModuleManager;

// Slate widget for rendering ImGui output.
class SImGuiWidget : public SLeafWidget
{
public:

	SLATE_BEGIN_ARGS(SImGuiWidget)
	{}
	SLATE_ARGUMENT(FImGuiModuleManager*, ModuleManager)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& WidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float) const override;

	FImGuiModuleManager* ModuleManager = nullptr;

	mutable TArray<FSlateVertex> VertexBuffer;
	mutable TArray<SlateIndex> IndexBuffer;
};
