// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "SImGuiCanvasControl.h"

#include "VersionCompatibility.h"

#include <Rendering/DrawElements.h>
#include <SlateOptMacros.h>


namespace
{
	// Mouse wheel to zoom ratio - how fast zoom changes with mouse wheel delta.
	const float ZoomScrollSpeed = 0.03125f;

	// Speed of blending out - how much zoom scale and canvas offset fades in every frame.
	const float BlendOutSpeed = 0.25f;

	// TODO: Move to settings
	namespace Colors
	{
		const FLinearColor CanvasMargin = FColor(0, 4, 32, 64);
		const FLinearColor CanvasBorder = FColor::Black.WithAlpha(127);
		const FLinearColor CanvasBorderHighlight = FColor(16, 24, 64, 160);
		const FLinearColor FrameBorder = FColor(222, 163, 9, 128);
		const FLinearColor FrameBorderHighlight = FColor(255, 180, 10, 160);
	}

	// Defines type of drag operation.
	enum class EDragType
	{
		Content,
		Canvas
	};

	// Data for drag & drop operations. Calculations are made in widget where we have more straightforward access to data
	// like geometry or scale.
	class FImGuiDragDropOperation : public FDragDropOperation
	{
	public:

		DRAG_DROP_OPERATOR_TYPE(FImGuiDragDropOperation, FDragDropOperation)

			FImGuiDragDropOperation(const FVector2D& InPosition, const FVector2D& InOffset, EDragType InDragType)
			: StartPosition(InPosition)
			, StartOffset(InOffset)
			, DragType(InDragType)
		{
			bCreateNewWindow = false;
			MouseCursor = EMouseCursor::GrabHandClosed;
		}

		FVector2D StartPosition;
		FVector2D StartOffset;
		EDragType DragType;
	};
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SImGuiCanvasControl::Construct(const FArguments& InArgs)
{
	OnTransformChanged = InArgs._OnTransformChanged;
	UpdateVisibility();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SImGuiCanvasControl::SetActive(bool bInActive)
{
	if (bActive != bInActive)
	{
		bActive = bInActive;
		bBlendingOut = !bInActive;
		if (bInActive)
		{
			Opacity = 1.f;
		}
		UpdateVisibility();
	}
}

void SImGuiCanvasControl::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	Super::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bBlendingOut)
	{
		if (FMath::IsNearlyEqual(CanvasScale, 1.f, ZoomScrollSpeed) && CanvasOffset.IsNearlyZero(1.f))
		{
			CanvasOffset = FVector2D::ZeroVector;
			CanvasScale = 1.f;
			Opacity = 0.f;
			bBlendingOut = false;
			UpdateVisibility();
		}
		else
		{
			CanvasOffset = FMath::Lerp(CanvasOffset, FVector2D::ZeroVector, BlendOutSpeed);
			CanvasScale = FMath::Lerp(CanvasScale, 1.f, BlendOutSpeed);
			Opacity = FMath::Lerp(Opacity, 0.f, BlendOutSpeed);
		}

		UpdateRenderTransform();
	}
}

FReply SImGuiCanvasControl::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (DragRequest == EDragRequest::None)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			DragRequest = EDragRequest::Content;
			return FReply::Handled().DetectDrag(SharedThis(this), EKeys::RightMouseButton).CaptureMouse(SharedThis(this));
		}
		else if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
		{
			DragRequest = EDragRequest::Canvas;
			return FReply::Handled().DetectDrag(SharedThis(this), EKeys::MiddleMouseButton).CaptureMouse(SharedThis(this));
		}
	}

	return FReply::Unhandled();
}

FReply SImGuiCanvasControl::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (DragRequest == EDragRequest::Content)
		{
			DragRequest = EDragRequest::None;
		}
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		if (DragRequest == EDragRequest::Canvas)
		{
			DragRequest = EDragRequest::None;
		}
	}

	return FReply::Unhandled();
}

FReply SImGuiCanvasControl::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Zoom(MyGeometry, MouseEvent.GetWheelDelta() * ZoomScrollSpeed, MouseEvent.GetScreenSpacePosition());
	return FReply::Unhandled();
}

FReply SImGuiCanvasControl::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (DragRequest == EDragRequest::Content)
	{
		return FReply::Handled()
			.BeginDragDrop(MakeShareable(new FImGuiDragDropOperation(
				MouseEvent.GetScreenSpacePosition(), ContentOffset, EDragType::Content)))
			.LockMouseToWidget(SharedThis(this));
	}
	else if (DragRequest == EDragRequest::Canvas)
	{
		return FReply::Handled()
			.BeginDragDrop(MakeShareable(new FImGuiDragDropOperation(
				MouseEvent.GetScreenSpacePosition(), CanvasOffset, EDragType::Canvas)))
			.LockMouseToWidget(SharedThis(this));
	}
	else
	{
		return FReply::Unhandled();
	}
}

FReply SImGuiCanvasControl::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	auto Operation = DragDropEvent.GetOperationAs<FImGuiDragDropOperation>();
	if (Operation.IsValid())
	{
		const FSlateRenderTransform ScreenToWidget = MyGeometry.GetAccumulatedRenderTransform().Inverse();
		const FVector2D DragDelta = ScreenToWidget.TransformVector(FVector2D(DragDropEvent.GetScreenSpacePosition() - Operation->StartPosition));
	
		if (Operation->DragType == EDragType::Content)
		{
			// Content offset is in ImGui space, so we need to scale drag calculated in widget space.
			ContentOffset = Operation->StartOffset + DragDelta / CanvasScale;
		}
		else
		{
			// Canvas offset is in widget space, so we can apply drag calculated in widget space directly.
			CanvasOffset = Operation->StartOffset + DragDelta;
		}

		UpdateRenderTransform();

		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}

FReply SImGuiCanvasControl::SImGuiCanvasControl::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	DragRequest = EDragRequest::None;
	return FReply::Handled().ReleaseMouseLock();
}

FVector2D SImGuiCanvasControl::ComputeDesiredSize(float InScale) const
{
	return FVector2D{ 3840.f, 2160.f } * InScale;
}

namespace
{
	FORCEINLINE FMargin CalculateInset(const FSlateRect& From, const FSlateRect& To)
	{
		return { To.Left - From.Left, To.Top - From.Top, From.Right - To.Right, From.Bottom - To.Bottom };
	}

	FORCEINLINE FLinearColor ScaleAlpha(FLinearColor Color, float Scale)
	{
		Color.A *= Scale;
		return Color;
	}

	FORCEINLINE FVector2D Round(const FVector2D& Vec)
	{
		return FVector2D{ FMath::FloorToFloat(Vec.X), FMath::FloorToFloat(Vec.Y) };
	}
}

int32 SImGuiCanvasControl::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();

	const FSlateRenderTransform& WidgetToScreen = AllottedGeometry.GetAccumulatedRenderTransform();
	const FSlateRenderTransform ImGuiToScreen = Transform.Concatenate(WidgetToScreen);

	const FSlateRect CanvasRect = FSlateRect(
		ImGuiToScreen.TransformPoint(FVector2D::ZeroVector),
		ImGuiToScreen.TransformPoint(ComputeDesiredSize(1.f)));
	const FMargin CanvasMargin = CalculateInset(MyCullingRect, CanvasRect);

	if (CanvasMargin.GetDesiredSize().SizeSquared() > 0.f)
	{
		CanvasBorderBrush.Margin = CanvasMargin;

		const FLinearColor CanvasMarginColor = ScaleAlpha(Colors::CanvasMargin, Opacity);
		const FLinearColor CanvasBorderColor = ScaleAlpha(DragRequest == EDragRequest::Content
			? Colors::CanvasBorderHighlight : Colors::CanvasBorder, Opacity);

#if ENGINE_COMPATIBILITY_LEGACY_CLIPPING_API
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, PaintGeometry, &CanvasBorderBrush, MyCullingRect,
			ESlateDrawEffect::None, CanvasMarginColor);

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, PaintGeometry, &CanvasBorderBrush,
			CanvasRect.ExtendBy(1).IntersectionWith(MyCullingRect), ESlateDrawEffect::None, CanvasBorderColor);

#else

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, PaintGeometry, &CanvasBorderBrush, ESlateDrawEffect::None,
			CanvasMarginColor);

		OutDrawElements.PushClip(FSlateClippingZone{ CanvasRect.ExtendBy(1) });
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, PaintGeometry, &CanvasBorderBrush, ESlateDrawEffect::None,
			CanvasBorderColor);
		OutDrawElements.PopClip();
#endif
	}

	const FSlateRect FrameRect = FSlateRect::FromPointAndExtent(
		WidgetToScreen.TransformPoint(Round(CanvasOffset)),
		Round(MyCullingRect.GetSize() * CanvasScale));
	const FMargin FrameMargin = CalculateInset(MyCullingRect, FrameRect);

	if (FrameMargin.GetDesiredSize().SizeSquared() > 0.f)
	{
		FrameBorderBrush.Margin = FrameMargin;

		const FLinearColor FrameBorderColor = ScaleAlpha(DragRequest == EDragRequest::Canvas
			? Colors::FrameBorderHighlight : Colors::FrameBorder, Opacity);

#if ENGINE_COMPATIBILITY_LEGACY_CLIPPING_API
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, PaintGeometry, &FrameBorderBrush,
			FrameRect.ExtendBy(1).IntersectionWith(MyCullingRect), ESlateDrawEffect::None, FrameBorderColor);
#else
		OutDrawElements.PushClip(FSlateClippingZone{ FrameRect.ExtendBy(1) });
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, PaintGeometry, &FrameBorderBrush, ESlateDrawEffect::None,
			FrameBorderColor);
		OutDrawElements.PopClip();
#endif
	}

	return LayerId;
}

void SImGuiCanvasControl::UpdateVisibility()
{
	SetVisibility(bActive ? EVisibility::Visible : bBlendingOut ? EVisibility::HitTestInvisible : EVisibility::Hidden);
}

void SImGuiCanvasControl::Zoom(const FGeometry& MyGeometry, const float Delta, const FVector2D& MousePosition)
{
	// If blending out, then cancel.
	bBlendingOut = false;

	float OldCanvasScale = CanvasScale;

	// Normalize scale to make sure that it changes in fixed steps and that we don't accumulate rounding errors.
	// Normalizing before applying delta allows for scales that at the edges are not rounded to the closes step.
	CanvasScale = FMath::RoundToFloat(CanvasScale / ZoomScrollSpeed) * ZoomScrollSpeed;

	// Update the scale.
	CanvasScale = FMath::Clamp(CanvasScale + Delta, GetMinScale(MyGeometry), 2.f);

	// Update canvas offset to keep it fixed around pivot point.
	if (CanvasScale != OldCanvasScale && OldCanvasScale != 0.f)
	{
		// Pivot points (in screen space):
		// 1) Around mouse: MousePosition
		// 2) Fixed in top-left corner: MyGeometry.GetLayoutBoundingRect().GetTopLeft()
		// 3) Fixed in centre: MyGeometry.GetLayoutBoundingRect().GetCenter()
		const FVector2D PivotPoint = MyGeometry.GetAccumulatedRenderTransform().Inverse().TransformPoint(MousePosition);
		const FVector2D Pivot = PivotPoint - CanvasOffset;

		CanvasOffset += Pivot * (OldCanvasScale - CanvasScale) / OldCanvasScale;
	}

	UpdateRenderTransform();
}

void SImGuiCanvasControl::UpdateRenderTransform()
{
	const FVector2D RenderOffset = Round(ContentOffset * CanvasScale + CanvasOffset);
	Transform = FSlateRenderTransform(CanvasScale, RenderOffset);
	OnTransformChanged.ExecuteIfBound(Transform);
}

float SImGuiCanvasControl::GetMinScale(const FGeometry& MyGeometry)
{
#if FROM_ENGINE_VERSION(4, 17)
#define GET_BOUNDING_RECT GetLayoutBoundingRect
#else
#define GET_BOUNDING_RECT GetClippingRect
#endif

	const FVector2D DefaultCanvasSize = MyGeometry.GetAccumulatedRenderTransform().TransformVector(ComputeDesiredSize(1.f));
	const FVector2D WidgetSize = MyGeometry.GET_BOUNDING_RECT().GetSize();
	return FMath::Min(WidgetSize.X / DefaultCanvasSize.X, WidgetSize.Y / DefaultCanvasSize.Y);

#undef GET_BOUNDING_RECT
}
