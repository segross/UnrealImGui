// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "SImGuiWidget.h"

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
	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.SetMouseDown(ImGuiInterops::GetMouseIndex(MouseEvent), true);
	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.SetMouseDown(ImGuiInterops::GetMouseIndex(MouseEvent), true);
	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.SetMouseDown(ImGuiInterops::GetMouseIndex(MouseEvent), false);
	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.AddMouseWheelDelta(MouseEvent.GetWheelDelta());
	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.SetMousePosition(MouseEvent.GetScreenSpacePosition() - MyGeometry.AbsolutePosition);
	return FReply::Handled();
}

void SImGuiWidget::OnPostImGuiUpdate()
{
	InputState.ClearUpdateState();
}

int32 SImGuiWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& WidgetStyle, bool bParentEnabled) const
{
	// Calculate offset that will transform vertex positions to screen space - rounded to avoid half pixel offsets.
	const FVector2D VertexPositionOffset{ FMath::RoundToFloat(MyClippingRect.Left), FMath::RoundToFloat(MyClippingRect.Top) };

	// Convert clipping rectangle to format required by Slate vertex.
	const FSlateRotatedRect VertexClippingRect{ MyClippingRect };

	for (const auto& DrawList : ModuleManager->GetContextProxy().GetDrawData())
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

	return LayerId;
}

FVector2D SImGuiWidget::ComputeDesiredSize(float) const
{
	return FVector2D{ 3840.f, 2160.f };
}
