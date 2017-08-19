// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiInputState.h"

#include <Widgets/SLeafWidget.h>


class FImGuiModuleManager;

// Slate widget for rendering ImGui output and storing Slate inputs.
class SImGuiWidget : public SLeafWidget
{
	typedef SLeafWidget Super;

public:

	SLATE_BEGIN_ARGS(SImGuiWidget)
	{}
	SLATE_ARGUMENT(FImGuiModuleManager*, ModuleManager)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	~SImGuiWidget();

	const FImGuiInputState& GetInputState() const { return InputState; }

	//----------------------------------------------------------------------------------------------------
	// SWidget overrides
	//----------------------------------------------------------------------------------------------------

	virtual bool SupportsKeyboardFocus() const override { return true; }

	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& CharacterEvent) override;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;

	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:

	FORCEINLINE void CopyModifierKeys(const FInputEvent& InputEvent);

	void OnPostImGuiUpdate();

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& WidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float) const override;

	FImGuiModuleManager* ModuleManager = nullptr;

	mutable TArray<FSlateVertex> VertexBuffer;
	mutable TArray<SlateIndex> IndexBuffer;

	FImGuiInputState InputState;
};
