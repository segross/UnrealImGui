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
		using FKeysArray = bool[ImGuiKey_NamedKey_COUNT];
		using FNavInputArray = float[ImGuiKey_GamepadRStickDown - ImGuiKey_GamepadStart + 1];
	}


	//====================================================================================================
	// Input Mapping
	//====================================================================================================

	// Map an Unreal key to a named ImGui key, independent of platform key codes.
	ImGuiKey ToImGuiKey(const FKey& Key);

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
	FORCEINLINE TextureIndex ToTextureIndex(ImTextureID Id)
	{
		const uint32 Slot = static_cast<uint32>(Id);
		return Slot == 0 || Slot > static_cast<uint32>(MAX_int32) ? INDEX_NONE : static_cast<TextureIndex>(Slot - 1);
	}

	// Convert from Texture Index to ImGui Texture Id that we pass to ImGui.
	FORCEINLINE ImTextureID ToImTextureID(TextureIndex Index, uint32 Generation = 0)
	{
		// Zero is reserved by Dear ImGui for an invalid texture.
		return Index == INDEX_NONE ? ImTextureID_Invalid : (static_cast<ImTextureID>(Generation) << 32) | (static_cast<ImTextureID>(Index) + 1);
	}
	FORCEINLINE uint32 ToTextureGeneration(ImTextureID Id) { return static_cast<uint32>(Id >> 32); }
}
