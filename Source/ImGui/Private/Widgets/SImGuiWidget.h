// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiInputState.h"
#include "ImGuiModuleDebug.h"
#include "ImGuiModuleSettings.h"

#include <Widgets/SCompoundWidget.h>


// Hide ImGui Widget debug in non-developer mode.
#define IMGUI_WIDGET_DEBUG IMGUI_MODULE_DEVELOPER

class FImGuiModuleManager;
class SImGuiCanvasControl;
class UImGuiInputHandler;

// Slate widget for rendering ImGui output and storing Slate inputs.
class SImGuiWidget : public SCompoundWidget
{
	typedef SCompoundWidget Super;

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

	//----------------------------------------------------------------------------------------------------
	// SWidget overrides
	//----------------------------------------------------------------------------------------------------

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual bool SupportsKeyboardFocus() const override { return bInputEnabled && !IsConsoleOpened(); }

	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& CharacterEvent) override;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;

	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;

	virtual FReply OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& AnalogInputEvent) override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& FocusEvent) override;

	virtual void OnFocusLost(const FFocusEvent& FocusEvent) override;

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

private:

	enum class EInputMode : uint8
	{
		None,
		// Mouse pointer only without user focus
		MousePointerOnly,
		// Full input with user focus (mouse, keyboard and depending on navigation mode gamepad)
		Full
	};

	void CreateInputHandler(const FStringClassReference& HandlerClassReference);
	void ReleaseInputHandler();

	void SetUseSoftwareCursor(bool bUse) { bUseSoftwareCursor = bUse; }

	void RegisterImGuiSettingsDelegates();
	void UnregisterImGuiSettingsDelegates();

	FORCEINLINE void CopyModifierKeys(const FInputEvent& InputEvent);
	FORCEINLINE void CopyModifierKeys(const FPointerEvent& MouseEvent);

	bool IsConsoleOpened() const;

	// Update visibility based on input enabled state.
	void UpdateVisibility();

	ULocalPlayer* GetLocalPlayer() const;
	void TakeFocus();
	void ReturnFocus();

	// Update input enabled state from console variable.
	void UpdateInputEnabled();

	// Determine new input mode based on hints.
	void UpdateInputMode(bool bHasKeyboardFocus, bool bHasMousePointer);

	void UpdateMouseStatus();

	FORCEINLINE bool HasMouseEventNotification() const { return bReceivedMouseEvent; }
	FORCEINLINE void NotifyMouseEvent() { bReceivedMouseEvent = true; }
	FORCEINLINE void ClearMouseEventNotification() { bReceivedMouseEvent = false; }

	void UpdateCanvasControlMode(const FInputEvent& InputEvent);

	void OnPostImGuiUpdate();

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& WidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float) const override;

	void SetImGuiTransform(const FSlateRenderTransform& Transform) { ImGuiTransform = Transform; }

#if IMGUI_WIDGET_DEBUG
	void OnDebugDraw();
#endif // IMGUI_WIDGET_DEBUG

	FImGuiModuleManager* ModuleManager = nullptr;
	TWeakObjectPtr<UGameViewportClient> GameViewport;
	TWeakObjectPtr<UImGuiInputHandler> InputHandler;

	FSlateRenderTransform ImGuiTransform;
	FSlateRenderTransform ImGuiRenderTransform;

	mutable TArray<FSlateVertex> VertexBuffer;
	mutable TArray<SlateIndex> IndexBuffer;

	int32 ContextIndex = 0;

	FImGuiInputState InputState;

	EInputMode InputMode = EInputMode::None;
	bool bInputEnabled = false;
	bool bReceivedMouseEvent = false;

	// Whether or not ImGui should draw its own cursor.
	bool bUseSoftwareCursor = false;

	TSharedPtr<SImGuiCanvasControl> CanvasControlWidget;
	TWeakPtr<SWidget> PreviousUserFocusedWidget;
};
