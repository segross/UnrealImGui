// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiModuleDebug.h"
#include "ImGuiModuleSettings.h"

#include <Rendering/RenderingCommon.h>
#include <UObject/WeakObjectPtr.h>
#include <Widgets/DeclarativeSyntaxSupport.h>
#include <Widgets/SCompoundWidget.h>


// Hide ImGui Widget debug in non-developer mode.
#define IMGUI_WIDGET_DEBUG IMGUI_MODULE_DEVELOPER

class FImGuiModuleManager;
class SImGuiCanvasControl;
class UImGuiInputHandler;

class UGameViewportClient;
class ULocalPlayer;

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

	virtual FReply OnTouchStarted(const FGeometry& MyGeometry, const FPointerEvent& TouchEvent) override;

	virtual FReply OnTouchMoved(const FGeometry& MyGeometry, const FPointerEvent& TouchEvent) override;

	virtual FReply OnTouchEnded(const FGeometry& MyGeometry, const FPointerEvent& TouchEvent) override;

private:

	void CreateInputHandler(const FSoftClassPath& HandlerClassReference);
	void ReleaseInputHandler();

	void RegisterImGuiSettingsDelegates();
	void UnregisterImGuiSettingsDelegates();

	void SetHideMouseCursor(bool bHide);

	bool IsConsoleOpened() const;

	// Update visibility based on input state.
	void UpdateVisibility();

	// Update cursor based on input state.
	void UpdateMouseCursor();

	ULocalPlayer* GetLocalPlayer() const;
	void TakeFocus();
	void ReturnFocus();

	// Update input state.
	void UpdateInputState();
	void UpdateTransparentMouseInput(const FGeometry& AllottedGeometry);
	void HandleWindowFocusLost();

	void SetDPIScale(const FImGuiDPIScaleInfo& ScaleInfo);

	void SetCanvasSizeInfo(const FImGuiCanvasSizeInfo& CanvasSizeInfo);
	void UpdateCanvasSize();

	void UpdateCanvasControlMode(const FInputEvent& InputEvent);

	void OnPostImGuiUpdate();

	FVector2D TransformScreenPointToImGui(const FGeometry& MyGeometry, const FVector2D& Point) const;

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

	FVector2D MinCanvasSize = FVector2D::ZeroVector;
	FVector2D CanvasSize = FVector2D::ZeroVector;

	float DPIScale = 1.f;

	bool bInputEnabled = false;
	bool bForegroundWindow = false;
	bool bHideMouseCursor = true;
	bool bTransparentMouseInput = false;
	bool bAdaptiveCanvasSize = false;
	bool bUpdateCanvasSize = false;
	bool bCanvasControlEnabled = false;

	TSharedPtr<SImGuiCanvasControl> CanvasControlWidget;
	TWeakPtr<SWidget> PreviousUserFocusedWidget;
};
