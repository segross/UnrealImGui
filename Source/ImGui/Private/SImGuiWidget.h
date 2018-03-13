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
	SLATE_ARGUMENT(UGameViewportClient*, GameViewport)
	SLATE_ARGUMENT(int32, ContextIndex)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	~SImGuiWidget();

	// Get index of the context that this widget is targeting.
	int32 GetContextIndex() const { return ContextIndex; }

	// Get input state associated with this widget.
	const FImGuiInputState& GetInputState() const { return InputState; }

	// Get the game viewport to which this widget is attached.
	const TWeakObjectPtr<UGameViewportClient>& GetGameViewport() const { return GameViewport; }

	// Detach widget from viewport assigned during construction (effectively allowing to dispose this widget). 
	void Detach();

	//----------------------------------------------------------------------------------------------------
	// SWidget overrides
	//----------------------------------------------------------------------------------------------------

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual bool SupportsKeyboardFocus() const override { return bInputEnabled; }

	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& CharacterEvent) override;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;

	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& FocusEvent) override;

	virtual void OnFocusLost(const FFocusEvent& FocusEvent) override;

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

private:

	enum class EInputMode : uint8
	{
		None,
		// Mouse pointer only without user focus
		MousePointerOnly,
		// Full input with user focus
		MouseAndKeyboard
	};

	FORCEINLINE void CopyModifierKeys(const FInputEvent& InputEvent);
	FORCEINLINE void CopyModifierKeys(const FPointerEvent& MouseEvent);

	bool IsConsoleOpened() const;

	bool IgnoreKeyEvent(const FKeyEvent& KeyEvent) const;

	// Update visibility based on input enabled state.
	void SetVisibilityFromInputEnabled();

	// Update input enabled state from console variable.
	void UpdateInputEnabled();

	// Determine new input mode based on hints.
	void UpdateInputMode(bool bHasKeyboardFocus, bool bHasMousePointer);

	void UpdateMouseStatus();

	FORCEINLINE bool HasMouseEventNotification() const { return bReceivedMouseEvent; }
	FORCEINLINE void NotifyMouseEvent() { bReceivedMouseEvent = true; }
	FORCEINLINE void ClearMouseEventNotification() { bReceivedMouseEvent = false; }

	void OnPostImGuiUpdate();

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& WidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float) const override;

	void OnDebugDraw();

	FImGuiModuleManager* ModuleManager = nullptr;
	TWeakObjectPtr<UGameViewportClient> GameViewport;

	mutable TArray<FSlateVertex> VertexBuffer;
	mutable TArray<SlateIndex> IndexBuffer;

	int32 ContextIndex = 0;

	EInputMode InputMode = EInputMode::None;
	bool bInputEnabled = false;
	bool bReceivedMouseEvent = false;

	FImGuiInputState InputState;

	TWeakPtr<SWidget> PreviousUserFocusedWidget;
};
