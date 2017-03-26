// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiDrawData.h"


void FImGuiDrawList::CopyVertexData(TArray<FSlateVertex>& OutVertexBuffer, const FVector2D VertexPositionOffset, const FSlateRotatedRect& VertexClippingRect) const
{
	// Reset and reserve space in destination buffer.
	OutVertexBuffer.SetNumUninitialized(ImGuiVertexBuffer.Size, false);

	// Transform and copy vertex data. 
	for (int Idx = 0; Idx < ImGuiVertexBuffer.Size; Idx++)
	{
		const ImDrawVert& ImGuiVertex = ImGuiVertexBuffer[Idx];
		FSlateVertex& SlateVertex = OutVertexBuffer[Idx];

		// Final UV is calculated in shader as XY * ZW, so we need set all components.
		SlateVertex.TexCoords[0] = ImGuiVertex.uv.x;
		SlateVertex.TexCoords[1] = ImGuiVertex.uv.y;
		SlateVertex.TexCoords[2] = SlateVertex.TexCoords[3] = 1.f;

		// Copy ImGui position and add offset.
		SlateVertex.Position[0] = ImGuiVertex.pos.x + VertexPositionOffset.X;
		SlateVertex.Position[1] = ImGuiVertex.pos.y + VertexPositionOffset.Y;

		// Set clipping rectangle.
		SlateVertex.ClipRect = VertexClippingRect;

		// Unpack ImU32 color.
		SlateVertex.Color = ImGuiInterops::UnpackImU32Color(ImGuiVertex.col);
	}
}

void FImGuiDrawList::CopyIndexData(TArray<SlateIndex>& OutIndexBuffer, const int32 StartIndex, const int32 NumElements) const
{
	// Reset buffer.
	OutIndexBuffer.SetNumUninitialized(NumElements, false);

	// Copy elements (slow copy because of different sizes of ImDrawIdx and SlateIndex and because SlateIndex can
	// have different size on different platforms).
	for (int i = 0; i < NumElements; i++)
	{
		OutIndexBuffer[i] = ImGuiIndexBuffer[StartIndex + i];
	}
}

void FImGuiDrawList::TransferDrawData(ImDrawList& Src)
{
	// Move data from source to this list.
	Src.CmdBuffer.swap(ImGuiCommandBuffer);
	Src.IdxBuffer.swap(ImGuiIndexBuffer);
	Src.VtxBuffer.swap(ImGuiVertexBuffer);

	// ImGui seems to clear draw lists in every frame, but since source list can contain pointers to buffers that
	// we just swapped, it is better to clear explicitly here.
	Src.Clear();
}
