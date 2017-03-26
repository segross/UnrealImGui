// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "SImGuiWidget.h"

#include "ImGuiModuleManager.h"
#include "TextureManager.h"
#include "Utilities/ScopeGuards.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SImGuiWidget::Construct(const FArguments& InArgs)
{
	checkf(InArgs._ModuleManager, TEXT("Null Module Manager argument"));
	ModuleManager = InArgs._ModuleManager;
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

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
