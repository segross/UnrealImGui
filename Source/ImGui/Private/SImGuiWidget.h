// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiInputState.h"
#include "ImGuiModuleDebug.h"
#include "ImGuiModuleSettings.h"

#include <Widgets/SLeafWidget.h>


// Hide ImGui Widget debug in non-developer mode.
#define IMGUI_WIDGET_DEBUG IMGUI_MODULE_DEVELOPER

class FImGuiModuleManager;
class UImGuiInputHandler;

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

	// If needed, add to event reply a mouse lock or unlock request.
	FORCEINLINE FReply WithMouseLockRequests(FReply&& Reply);

	FORCEINLINE void CopyModifierKeys(const FInputEvent& InputEvent);
	FORCEINLINE void CopyModifierKeys(const FPointerEvent& MouseEvent);

	bool IsConsoleOpened() const;

	void SetMouseCursorOverride(EMouseCursor::Type InMouseCursorOverride);

	// Update visibility based on input enabled state.
	void SetVisibilityFromInputEnabled();

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

	void OnPostImGuiUpdate();

	// Update canvas map mode based on input state.
	void UpdateCanvasMapMode(const FInputEvent& InputEvent);
	void SetCanvasMapMode(bool bEnabled);

	void AddCanvasScale(float Delta);
	void UdateCanvasScale(float DeltaSeconds);

	void UpdateCanvasDraggingConditions(const FPointerEvent& MouseEvent);
	void UpdateCanvasDragging(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	// Canvas scale in which the whole canvas is visible in the viewport. We don't scale below that value.
	float GetMinCanvasScale() const;

	// Normalized canvas scale mapping range [MinCanvasScale..1] to [0..1].
	float GetNormalizedCanvasScale(float Scale) const;

	// Position of the canvas origin, given the current canvas scale and offset. Uses NormalizedCanvasScale to smoothly
	// transition between showing visible canvas area at scale 1 and the whole canvas at min canvas scale. 
	FVector2D GetCanvasPosition(float Scale, const FVector2D& Offset) const;

	bool InFrameGrabbingRange(const FVector2D& Position, float Scale, const FVector2D& Offset) const;

	FVector2D GetViewportSize() const;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& WidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float) const override;

#if IMGUI_WIDGET_DEBUG
	void OnDebugDraw();
#endif // IMGUI_WIDGET_DEBUG

	FImGuiModuleManager* ModuleManager = nullptr;
	TWeakObjectPtr<UGameViewportClient> GameViewport;
	TWeakObjectPtr<UImGuiInputHandler> InputHandler;

	mutable TArray<FSlateVertex> VertexBuffer;
	mutable TArray<SlateIndex> IndexBuffer;

	int32 ContextIndex = 0;

	FImGuiInputState InputState;

	EInputMode InputMode = EInputMode::None;
	bool bInputEnabled = false;
	bool bReceivedMouseEvent = false;
	bool bMouseLock = false;

	// Whether or not ImGui should draw its own cursor.
	bool bUseSoftwareCursor = false;

	// Canvas map mode allows to zoom in/out and navigate between different parts of ImGui canvas.
	bool bCanvasMapMode = false;

	// If enabled (only if not fully zoomed out), allows to drag ImGui canvas. Dragging canvas modifies canvas offset.
	bool bCanvasDragging = false;

	// If enabled (only if zoomed out), allows to drag a frame that represents a visible area of the ImGui canvas.
	// Mouse deltas are converted to canvas offset by linear formula derived from GetCanvasPosition function.
	bool bFrameDragging = false;

	// True, if mouse and input are in state that allows to start frame dragging. Used for highlighting.
	bool bFrameDraggingReady = false;

	bool bFrameDraggingSkipMouseMove = false;

	EMouseCursor::Type MouseCursorOverride = EMouseCursor::None;

	float TargetCanvasScale = 1.f;

	float CanvasScale = 1.f;
	FVector2D CanvasOffset = FVector2D::ZeroVector;

	float ImGuiFrameCanvasScale = 1.f;
	FVector2D ImGuiFrameCanvasOffset = FVector2D::ZeroVector;

	TWeakPtr<SWidget> PreviousUserFocusedWidget;
};
