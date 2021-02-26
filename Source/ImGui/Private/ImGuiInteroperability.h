// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "TextureManager.h"

#include <GenericPlatform/ICursor.h>
#include <Input/Events.h>

#include <imgui.h>


class FImGuiInputState;

// Utilities to help standardise operations between Unreal and ImGui.
namespace ImGuiInterops
{
	//====================================================================================================
	// ImGui Types
	//====================================================================================================

	namespace ImGuiTypes
	{
		using FMouseButtonsArray = decltype(ImGuiIO::MouseDown);
		using FKeysArray = decltype(ImGuiIO::KeysDown);
		using FNavInputArray = decltype(ImGuiIO::NavInputs);

		using FKeyMap = decltype(ImGuiIO::KeyMap);
	}


	//====================================================================================================
	// Input Mapping
	//====================================================================================================

	// Set in ImGui IO mapping to recognize indices generated from Unreal input events.
	void SetUnrealKeyMap(ImGuiIO& IO);

	// Map FKey to index in keys buffer.
	uint32 GetKeyIndex(const FKey& Key);

	// Map key event to index in keys buffer.
	uint32 GetKeyIndex(const FKeyEvent& KeyEvent);

	// Map mouse FKey to index in mouse buttons buffer.
	uint32 GetMouseIndex(const FKey& MouseButton);

	// Map pointer event to index in mouse buttons buffer.
	FORCEINLINE uint32 GetMouseIndex(const FPointerEvent& MouseEvent)
	{
		return GetMouseIndex(MouseEvent.GetEffectingButton());
	}

	// Convert from ImGuiMouseCursor type to EMouseCursor.
	EMouseCursor::Type ToSlateMouseCursor(ImGuiMouseCursor MouseCursor);

	// Set in the target array navigation input corresponding to gamepad key.
	// @param NavInputs - Target array
	// @param Key - Gamepad key mapped to navigation input (non-mapped keys will be ignored)
	// @param bIsDown - True, if key is down 
	void SetGamepadNavigationKey(ImGuiTypes::FNavInputArray& NavInputs, const FKey& Key, bool bIsDown);

	// Set in the target array navigation input corresponding to gamepad axis.
	// @param NavInputs - Target array
	// @param Key - Gamepad axis key mapped to navigation input (non-axis or non-mapped inputs will be ignored)
	// @param Value - Axis value (-1..1 values from Unreal are mapped to separate ImGui axes with values in range 0..1)
	void SetGamepadNavigationAxis(ImGuiTypes::FNavInputArray& NavInputs, const FKey& Key, float Value);


	//====================================================================================================
	// Input State Copying
	//====================================================================================================

	// Copy input to ImGui IO.
	// @param IO - Target ImGui IO
	// @param InputState - Input state to copy
	void CopyInput(ImGuiIO& IO, const FImGuiInputState& InputState);


	//====================================================================================================
	// Conversions
	//====================================================================================================

	// Convert from ImGui packed color to FColor.
	FORCEINLINE FColor UnpackImU32Color(ImU32 Color)
	{
		// We use IM_COL32_R/G/B/A_SHIFT macros to support different ImGui configurations.
		return FColor{ (uint8)((Color >> IM_COL32_R_SHIFT) & 0xFF), (uint8)((Color >> IM_COL32_G_SHIFT) & 0xFF),
			(uint8)((Color >> IM_COL32_B_SHIFT) & 0xFF), (uint8)((Color >> IM_COL32_A_SHIFT) & 0xFF) };
	}

	// Convert from ImVec4 rectangle to FSlateRect.
	FORCEINLINE FSlateRect ToSlateRect(const ImVec4& ImGuiRect)
	{
		return FSlateRect{ ImGuiRect.x, ImGuiRect.y, ImGuiRect.z, ImGuiRect.w };
	}

	// Convert from ImVec2 rectangle to FVector2D.
	FORCEINLINE FVector2D ToVector2D(const ImVec2& ImGuiVector)
	{
		return FVector2D{ ImGuiVector.x, ImGuiVector.y };
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
