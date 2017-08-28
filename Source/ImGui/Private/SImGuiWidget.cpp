// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "SImGuiWidget.h"

#include "ImGuiContextManager.h"
#include "ImGuiContextProxy.h"
#include "ImGuiInteroperability.h"
#include "ImGuiModuleManager.h"
#include "TextureManager.h"
#include "Utilities/ScopeGuards.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SImGuiWidget::Construct(const FArguments& InArgs)
{
	checkf(InArgs._ModuleManager, TEXT("Null Module Manager argument"));
	ModuleManager = InArgs._ModuleManager;
	ContextIndex = InArgs._ContextIndex;

	ModuleManager->OnPostImGuiUpdate().AddRaw(this, &SImGuiWidget::OnPostImGuiUpdate);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SImGuiWidget::~SImGuiWidget()
{
	ModuleManager->OnPostImGuiUpdate().RemoveAll(this);
}

FReply SImGuiWidget::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& CharacterEvent)
{
	InputState.AddCharacter(CharacterEvent.GetCharacter());

	return FReply::Handled();
}

FReply SImGuiWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
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
	InputState.SetKeyDown(ImGuiInterops::GetKeyIndex(KeyEvent), false);
	CopyModifierKeys(KeyEvent);

	return FReply::Handled();
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

	return FReply::Handled();
}

FReply SImGuiWidget::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& FocusEvent)
{
	Super::OnFocusReceived(MyGeometry, FocusEvent);

	// If widget has a keyboard focus we always maintain mouse input. Technically, if mouse is outside of the widget
	// area it won't generate events but we freeze its state until it either comes back or input is completely lost.
	UpdateInputMode(true, true);

	return FReply::Handled();
}

void SImGuiWidget::OnFocusLost(const FFocusEvent& FocusEvent)
{
	Super::OnFocusLost(FocusEvent);

	UpdateInputMode(false, IsHovered());
}

void SImGuiWidget::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Super::OnMouseEnter(MyGeometry, MouseEvent);

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
	if (InputMode == EInputMode::MouseOnly)
	{
		CopyModifierKeys(static_cast<const FInputEvent&>(MouseEvent));
	}
}

void SImGuiWidget::UpdateInputMode(bool bNeedKeyboard, bool bNeedMouse)
{
	const EInputMode NewInputMode =
		bNeedKeyboard ? EInputMode::MouseAndKeyboard :
		bNeedMouse ? EInputMode::MouseOnly :
		EInputMode::None;

	if (InputMode != NewInputMode)
	{
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
