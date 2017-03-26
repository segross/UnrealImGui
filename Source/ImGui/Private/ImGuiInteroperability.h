// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "TextureManager.h"

#include <imgui.h>


// Utilities to help standardise operations between Unreal and ImGui.
namespace ImGuiInterops
{
	//====================================================================================================
	// Conversions
	//====================================================================================================

	// Convert from ImGui packed color to FColor.
	FORCEINLINE FColor UnpackImU32Color(ImU32 Color)
	{
		// We use IM_COL32_R/G/B/A_SHIFT macros to support different ImGui configurations.
		return FColor{ ((Color >> IM_COL32_R_SHIFT) & 0xFF), ((Color >> IM_COL32_G_SHIFT) & 0xFF),
			((Color >> IM_COL32_B_SHIFT) & 0xFF), ((Color >> IM_COL32_A_SHIFT) & 0xFF) };
	}

	// Convert from ImVec4 rectangle to FSlateRect.
	FORCEINLINE FSlateRect ToSlateRect(const ImVec4& ImGuiRect)
	{
		return FSlateRect{ ImGuiRect.x, ImGuiRect.y, ImGuiRect.z, ImGuiRect.w };
	}

	// Convert from ImGui Texture Id to Texture Index that we use for texture resources.
	FORCEINLINE TextureIndex ToTextureIndex(ImTextureID Index)
	{
		return static_cast<TextureIndex>(reinterpret_cast<intptr_t>(Index));
	}

	// Convert from Texture Index to ImGui Texture Id that we pass to ImGui.
	FORCEINLINE ImTextureID ToImTextureID(TextureIndex Index)
	{
		return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(Index));
	}
}
