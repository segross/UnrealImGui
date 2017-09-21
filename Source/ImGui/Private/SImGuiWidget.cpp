// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "SImGuiWidget.h"

#include "ImGuiContextManager.h"
#include "ImGuiContextProxy.h"
#include "ImGuiInteroperability.h"
#include "ImGuiModuleManager.h"
#include "TextureManager.h"
#include "Utilities/ScopeGuards.h"

#include <Engine/Console.h>


// High enough z-order guarantees that ImGui output is rendered on top of the game UI.
constexpr int32 IMGUI_WIDGET_Z_ORDER = 10000;


DEFINE_LOG_CATEGORY_STATIC(LogImGuiWidget, Warning, All);

#define TEXT_INPUT_MODE(Val) (\
	(Val) == EInputMode::MouseAndKeyboard ? TEXT("MouseAndKeyboard") :\
	(Val) == EInputMode::MousePointerOnly ? TEXT("MousePointerOnly") :\
	TEXT("None"))

#define TEXT_BOOL(Val) ((Val) ? TEXT("true") : TEXT("false"))


namespace CVars
{
	TAutoConsoleVariable<int> InputEnabled(TEXT("ImGui.InputEnabled"), 0,
		TEXT("Allows to enable or disable ImGui input mode.\n")
		TEXT("0: disabled (default)\n")
		TEXT("1: enabled, input is routed to ImGui and with a few exceptions is consumed."),
		ECVF_Default);

	TAutoConsoleVariable<int> DebugWidget(TEXT("ImGui.Debug.Widget"), 0,
		TEXT("Show debug for SImGuiWidget.\n")
		TEXT("0: disabled (default)\n")
		TEXT("1: enabled."),
		ECVF_Default);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SImGuiWidget::Construct(const FArguments& InArgs)
{
	checkf(InArgs._ModuleManager, TEXT("Null Module Manager argument"));

	ModuleManager = InArgs._ModuleManager;
	ContextIndex = InArgs._ContextIndex;

	// Disable mouse cursor over this widget as we will use ImGui to draw it.
	SetCursor(EMouseCursor::None);

	// Sync visibility with default input enabled state.
	SetVisibilityFromInputEnabled();

	// Register to get post-update notifications, so we can clean frame updates.
	ModuleManager->OnPostImGuiUpdate().AddRaw(this, &SImGuiWidget::OnPostImGuiUpdate);

	// Register self-debug function.
	auto* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex);
	checkf(ContextProxy, TEXT("Missing context during widget construction: ContextIndex = %d"), ContextIndex);
	ContextProxy->OnDraw().AddRaw(this, &SImGuiWidget::OnDebugDraw);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SImGuiWidget::~SImGuiWidget()
{
	// Remove binding between this widget and its context proxy.
	if (auto* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex))
	{
		ContextProxy->OnDraw().RemoveAll(this);
	}

	// Unregister from post-update notifications.
	ModuleManager->OnPostImGuiUpdate().RemoveAll(this);
}

void SImGuiWidget::AttachToViewport(UGameViewportClient* InGameViewport, bool bResetInput)
{
	checkf(InGameViewport, TEXT("Null InGameViewport"));
	checkf(!GameViewport.IsValid() || GameViewport.Get() == InGameViewport,
		TEXT("Widget is attached to another game viewport and will be available for reuse only after this session ")
		TEXT("ends. ContextIndex = %d, CurrentGameViewport = %s, InGameViewport = %s"),
		ContextIndex, *GameViewport->GetName(), InGameViewport->GetName());

	if (bResetInput)
	{
		ResetInputState();
	}

	GameViewport = InGameViewport;
	GameViewport->AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(SharedThis(this)), IMGUI_WIDGET_Z_ORDER);
}

void SImGuiWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	Super::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UpdateMouseStatus();

	// Note: Moving that update to console variable sink or callback might seem like a better alternative but input
	// setup in this function is better handled here.
	UpdateInputEnabled();
}

FReply SImGuiWidget::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& CharacterEvent)
{
	if (IsConsoleOpened())
	{
		return FReply::Unhandled();
	}

	InputState.AddCharacter(CharacterEvent.GetCharacter());

	return FReply::Handled();
}

FReply SImGuiWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (IsConsoleOpened() || IgnoreKeyEvent(KeyEvent))
	{
		return FReply::Unhandled();
	}

	InputState.SetKeyDown(ImGuiInterops::GetKeyIndex(KeyEvent), true);
	CopyModifierKeys(KeyEvent);

	// If this is tilde key then let input through and release the focus to allow console to process it.
	if (KeyEvent.GetKey() == EKeys::Tilde)
	{
		return FReply::Unhandled();
	}

	return FReply::Handled();
}

FReply SImGuiWidget::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	// Even if we don't send new keystrokes to ImGui, we still handle key up events, to make sure that we clear keys
	// pressed before suppressing keyboard input.
	InputState.SetKeyDown(ImGuiInterops::GetKeyIndex(KeyEvent), false);
	CopyModifierKeys(KeyEvent);

	// If console is opened we notify key change but we also let event trough, so it can be handled by console.
	return IsConsoleOpened() ? FReply::Unhandled() : FReply::Handled();
}

FReply SImGuiWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.SetMouseDown(ImGuiInterops::GetMouseIndex(MouseEvent), true);
	CopyModifierKeys(MouseEvent);

	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.SetMouseDown(ImGuiInterops::GetMouseIndex(MouseEvent), true);
	CopyModifierKeys(MouseEvent);

	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.SetMouseDown(ImGuiInterops::GetMouseIndex(MouseEvent), false);
	CopyModifierKeys(MouseEvent);

	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.AddMouseWheelDelta(MouseEvent.GetWheelDelta());
	CopyModifierKeys(MouseEvent);

	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.SetMousePosition(MouseEvent.GetScreenSpacePosition() - MyGeometry.AbsolutePosition);
	CopyModifierKeys(MouseEvent);

	// This event is called in every frame when we have a mouse, so we can use it to raise notifications.
	NotifyMouseEvent();

	return FReply::Handled();
}

FReply SImGuiWidget::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& FocusEvent)
{
	Super::OnFocusReceived(MyGeometry, FocusEvent);

	UE_LOG(LogImGuiWidget, VeryVerbose, TEXT("ImGui Widget %d - Focus Received."), ContextIndex);

	// If widget has a keyboard focus we always maintain mouse input. Technically, if mouse is outside of the widget
	// area it won't generate events but we freeze its state until it either comes back or input is completely lost.
	UpdateInputMode(true, IsDirectlyHovered());

	FSlateApplication::Get().ResetToDefaultPointerInputSettings();
	return FReply::Handled();
}

void SImGuiWidget::OnFocusLost(const FFocusEvent& FocusEvent)
{
	Super::OnFocusLost(FocusEvent);

	UE_LOG(LogImGuiWidget, VeryVerbose, TEXT("ImGui Widget %d - Focus Lost."), ContextIndex);

	UpdateInputMode(false, IsDirectlyHovered());
}

void SImGuiWidget::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Super::OnMouseEnter(MyGeometry, MouseEvent);

	UE_LOG(LogImGuiWidget, VeryVerbose, TEXT("ImGui Widget %d - Mouse Enter."), ContextIndex);

	// If mouse enters while input is active then we need to update mouse buttons because there is a chance that we
	// missed some events.
	if (InputMode != EInputMode::None)
	{
		for (const FKey& Button : { EKeys::LeftMouseButton, EKeys::MiddleMouseButton, EKeys::RightMouseButton, EKeys::ThumbMouseButton, EKeys::ThumbMouseButton2 })
		{
			InputState.SetMouseDown(ImGuiInterops::GetMouseIndex(Button), MouseEvent.IsMouseButtonDown(Button));
		}
	}

	UpdateInputMode(HasKeyboardFocus(), true);
}

void SImGuiWidget::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	Super::OnMouseLeave(MouseEvent);

	UE_LOG(LogImGuiWidget, VeryVerbose, TEXT("ImGui Widget %d - Mouse Leave."), ContextIndex);

	UpdateInputMode(HasKeyboardFocus(), false);
}

void SImGuiWidget::CopyModifierKeys(const FInputEvent& InputEvent)
{
	InputState.SetControlDown(InputEvent.IsControlDown());
	InputState.SetShiftDown(InputEvent.IsShiftDown());
	InputState.SetAltDown(InputEvent.IsAltDown());
}

void SImGuiWidget::CopyModifierKeys(const FPointerEvent& MouseEvent)
{
	if (InputMode == EInputMode::MousePointerOnly)
	{
		CopyModifierKeys(static_cast<const FInputEvent&>(MouseEvent));
	}
}

bool SImGuiWidget::IsConsoleOpened() const
{
	return GameViewport->ViewportConsole && GameViewport->ViewportConsole->ConsoleState != NAME_None;
}

bool SImGuiWidget::IgnoreKeyEvent(const FKeyEvent& KeyEvent) const
{
	// Ignore console open/close events.
	if (KeyEvent.GetKey() == EKeys::Tilde)
	{
		return true;
	}

	// Ignore escape keys unless they are needed to cancel operations in ImGui.
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		auto* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex);
		if (!ContextProxy || !ContextProxy->HasActiveItem())
		{
			return true;
		}
	}

	return false;
}

void SImGuiWidget::ResetInputState()
{
	bInputEnabled = false;
	SetVisibilityFromInputEnabled();
	UpdateInputMode(false, false);
}

void SImGuiWidget::SetVisibilityFromInputEnabled()
{
	// If we don't use input disable hit test to make this widget invisible for cursors hit detection.
	SetVisibility(bInputEnabled ? EVisibility::Visible : EVisibility::HitTestInvisible);

	UE_LOG(LogImGuiWidget, VeryVerbose, TEXT("ImGui Widget %d - Visibility updated to '%s'."),
		ContextIndex, *GetVisibility().ToString());
}

void SImGuiWidget::UpdateInputEnabled()
{
	const bool bEnabled = CVars::InputEnabled.GetValueOnGameThread() > 0;
	if (bInputEnabled != bEnabled)
	{
		bInputEnabled = bEnabled;

		UE_LOG(LogImGuiWidget, Log, TEXT("ImGui Widget %d - Input Enabled changed to '%s'."),
			ContextIndex, TEXT_BOOL(bInputEnabled));

		SetVisibilityFromInputEnabled();

		if (!bInputEnabled)
		{
			auto& Slate = FSlateApplication::Get();
			if (Slate.GetKeyboardFocusedWidget().Get() == this)
			{
				Slate.ResetToDefaultPointerInputSettings();
				Slate.SetUserFocus(Slate.GetUserIndexForKeyboard(),
					PreviousUserFocusedWidget.IsValid() ? PreviousUserFocusedWidget.Pin() : GameViewport->GetGameViewportWidget());
			}

			PreviousUserFocusedWidget.Reset();

			UpdateInputMode(false, false);
		}
	}

	// Note: Some widgets, like console, can reset focus to viewport after we already grabbed it. If we detect that
	// viewport has a focus while input is enabled we will take it.
	if (bInputEnabled && !HasKeyboardFocus() && !IsConsoleOpened())
	{
		const auto& ViewportWidget = GameViewport->GetGameViewportWidget();
		if (ViewportWidget->HasKeyboardFocus() || ViewportWidget->HasFocusedDescendants())
		{
			auto& Slate = FSlateApplication::Get();
			PreviousUserFocusedWidget = Slate.GetUserFocusedWidget(Slate.GetUserIndexForKeyboard());
			Slate.SetKeyboardFocus(SharedThis(this));
		}
	}
}

void SImGuiWidget::UpdateInputMode(bool bHasKeyboardFocus, bool bHasMousePointer)
{
	const EInputMode NewInputMode =
		bHasKeyboardFocus ? EInputMode::MouseAndKeyboard :
		bHasMousePointer ? EInputMode::MousePointerOnly :
		EInputMode::None;

	if (InputMode != NewInputMode)
	{
		UE_LOG(LogImGuiWidget, Verbose, TEXT("ImGui Widget %d - Input Mode changed from '%s' to '%s'."),
			ContextIndex, TEXT_INPUT_MODE(InputMode), TEXT_INPUT_MODE(NewInputMode));

		// We need to reset input components if we are either fully shutting down or we are downgrading from full to
		// mouse-only input mode.
		if (NewInputMode == EInputMode::None)
		{
			InputState.ResetState();
		}
		else if (InputMode == EInputMode::MouseAndKeyboard)
		{
			InputState.ResetKeyboardState();
		}

		InputMode = NewInputMode;

		ClearMouseEventNotification();
	}

	InputState.SetMousePointer(bHasMousePointer);
}

void SImGuiWidget::UpdateMouseStatus()
{
	// Note: Mouse leave events can get lost if other viewport takes mouse capture (for instance console is opened by
	// different viewport when this widget is hovered). With that we lose a chance to cleanup and hide ImGui pointer.
	// We could either update ImGui pointer in every frame or like below, use mouse events to catch when mouse is lost.

	if (InputMode == EInputMode::MousePointerOnly)
	{
		if (!HasMouseEventNotification())
		{
			UpdateInputMode(false, IsDirectlyHovered());
		}
		ClearMouseEventNotification();
	}
}

void SImGuiWidget::OnPostImGuiUpdate()
{
	if (InputMode != EInputMode::None)
	{
		InputState.ClearUpdateState();
	}
}

int32 SImGuiWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& WidgetStyle, bool bParentEnabled) const
{
	if (FImGuiContextProxy* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex))
	{
		// Calculate offset that will transform vertex positions to screen space - rounded to avoid half pixel offsets.
		const FVector2D VertexPositionOffset{ FMath::RoundToFloat(MyClippingRect.Left), FMath::RoundToFloat(MyClippingRect.Top) };

		// Convert clipping rectangle to format required by Slate vertex.
		const FSlateRotatedRect VertexClippingRect{ MyClippingRect };

		for (const auto& DrawList : ContextProxy->GetDrawData())
		{
			DrawList.CopyVertexData(VertexBuffer, VertexPositionOffset, VertexClippingRect);

			// Get access to the Slate scissor rectangle defined in Slate Core API, so we can customize elements drawing.
			extern SLATECORE_API TOptional<FShortRect> GSlateScissorRect;

			auto GSlateScissorRectSaver = ScopeGuards::MakeStateSaver(GSlateScissorRect);

			int IndexBufferOffset = 0;
			for (int CommandNb = 0; CommandNb < DrawList.NumCommands(); CommandNb++)
			{
				const auto& DrawCommand = DrawList.GetCommand(CommandNb);

				DrawList.CopyIndexData(IndexBuffer, IndexBufferOffset, DrawCommand.NumElements);

				// Advance offset by number of copied elements to position it for the next command.
				IndexBufferOffset += DrawCommand.NumElements;

				// Get texture resource handle for this draw command (null index will be also mapped to a valid texture).
				const FSlateResourceHandle& Handle = ModuleManager->GetTextureManager().GetTextureHandle(DrawCommand.TextureId);

				// Transform clipping rectangle to screen space and set in Slate, to apply it to elements that we draw.
				GSlateScissorRect = FShortRect{ DrawCommand.ClippingRect.OffsetBy(MyClippingRect.GetTopLeft()).IntersectionWith(MyClippingRect) };

				// Add elements to the list.
				FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, Handle, VertexBuffer, IndexBuffer, nullptr, 0, 0);
			}
		}
	}

	return LayerId;
}

FVector2D SImGuiWidget::ComputeDesiredSize(float) const
{
	return FVector2D{ 3840.f, 2160.f };
}

// Controls tweaked for 2-columns layout.
namespace TwoColumns
{
	static void GroupName(const char* Name)
	{
		ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.f }, Name); ImGui::NextColumn(); ImGui::NextColumn();
	}

	static void Value(const char* Label, int Value)
	{
		ImGui::Text("%s:", Label); ImGui::NextColumn();
		ImGui::Text("%d", Value); ImGui::NextColumn();
	}

	static void Value(const char* Label, bool bValue)
	{
		ImGui::Text("%s:", Label); ImGui::NextColumn();
		ImGui::Text("%ls", TEXT_BOOL(bValue)); ImGui::NextColumn();
	}

	static void Value(const char* Label, const TCHAR* Value)
	{
		ImGui::Text("%s:", Label); ImGui::NextColumn();
		ImGui::Text("%ls", Value); ImGui::NextColumn();
	}
}

void SImGuiWidget::OnDebugDraw()
{
	bool bDebug = CVars::DebugWidget.GetValueOnGameThread() > 0;
	if (bDebug)
	{
		ImGui::SetNextWindowSize(ImVec2(380, 340), ImGuiSetCond_Once);
		if (ImGui::Begin("ImGui Widget Debug", &bDebug))
		{
			ImGui::Columns(2, nullptr, false);

			TwoColumns::Value("Context Index", ContextIndex);
			TwoColumns::Value("Game Viewport", *GameViewport->GetName());

			ImGui::Separator();

			TwoColumns::Value("Input Enabled", bInputEnabled);
			TwoColumns::Value("Input Mode", TEXT_INPUT_MODE(InputMode));
			TwoColumns::Value("Input Has Mouse Pointer", InputState.HasMousePointer());

			ImGui::Separator();

			const float GroupIndent = 5.f;

			TwoColumns::GroupName("Widget");
			ImGui::Indent(GroupIndent);
			{
				TwoColumns::Value("Visibility", *GetVisibility().ToString());
				TwoColumns::Value("Is Hovered", IsHovered());
				TwoColumns::Value("Is Directly Hovered", IsDirectlyHovered());
				TwoColumns::Value("Has Keyboard Input", HasKeyboardFocus());
			}
			ImGui::Unindent(GroupIndent);

			ImGui::Separator();

			TwoColumns::GroupName("Viewport Widget");
			ImGui::Indent(GroupIndent);
			{
				const auto& ViewportWidget = GameViewport->GetGameViewportWidget();
				TwoColumns::Value("Is Hovered", ViewportWidget->IsHovered());
				TwoColumns::Value("Is Directly Hovered", ViewportWidget->IsDirectlyHovered());
				TwoColumns::Value("Has Mouse Capture", ViewportWidget->HasMouseCapture());
				TwoColumns::Value("Has Keyboard Input", ViewportWidget->HasKeyboardFocus());
				TwoColumns::Value("Has Focused Descendants", ViewportWidget->HasFocusedDescendants());
				auto Widget = PreviousUserFocusedWidget.Pin();
				TwoColumns::Value("Previous User Focused", Widget.IsValid() ? *Widget->GetTypeAsString() : TEXT("None"));
			}
			ImGui::Unindent(GroupIndent);

			ImGui::Columns(1);
		}
		ImGui::End();

		if (!bDebug)
		{
			CVars::DebugWidget->ClearFlags(ECVF_SetByConsole);
			CVars::DebugWidget->Set(0);
		}
	}
}

#undef TEXT_INPUT_MODE
#undef TEXT_BOOL
