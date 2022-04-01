// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiInteroperability.h"
#include "Utilities/Arrays.h"

#include <Containers/Array.h>


// Collects and stores input state and updates for ImGui IO.
class FImGuiInputState
{
public:

	// Characters buffer.
	using FCharactersBuffer = TArray<TCHAR, TInlineAllocator<8>>;

	// Array for mouse button states.
	using FMouseButtonsArray = ImGuiInterops::ImGuiTypes::FMouseButtonsArray;

	// Array for key states.
	using FKeysArray = ImGuiInterops::ImGuiTypes::FKeysArray;

	// Array for navigation input states.
	using FNavInputArray = ImGuiInterops::ImGuiTypes::FNavInputArray;

	// Pair of indices defining range in mouse buttons array.
	using FMouseButtonsIndexRange = Utilities::TArrayIndexRange<FMouseButtonsArray, uint32>;

	// Pair of indices defining range in keys array.
	using FKeysIndexRange = Utilities::TArrayIndexRange<FKeysArray, uint32>;

	// Create empty state with whole range instance with the whole update state marked as dirty.
	FImGuiInputState();

	// Get reference to input characters buffer.
	const FCharactersBuffer& GetCharacters() const { return InputCharacters; }

	// Add a character to the characters buffer. We can store and send to ImGui up to 16 characters per frame. Any
	// character beyond that limit will be discarded.
	// @param Char - Character to add
	void AddCharacter(TCHAR Char);

	// Get reference to the array with key down states.
	const FKeysArray& GetKeys() const { return KeysDown; }

	// Get possibly empty range of indices bounding dirty part of the keys array.
	const FKeysIndexRange& GetKeysUpdateRange() const { return KeysUpdateRange; }

	// Change state of the key in the keys array and expand range bounding dirty part of the array.
	// @param KeyEvent - Key event representing the key
	// @param bIsDown - True, if key is down
	void SetKeyDown(const FKeyEvent& KeyEvent, bool bIsDown) { SetKeyDown(ImGuiInterops::GetKeyIndex(KeyEvent), bIsDown); }

	// Change state of the key in the keys array and expand range bounding dirty part of the array.
	// @param Key - Keyboard key
	// @param bIsDown - True, if key is down
	void SetKeyDown(const FKey& Key, bool bIsDown) { SetKeyDown(ImGuiInterops::GetKeyIndex(Key), bIsDown); }

	// Get reference to the array with mouse button down states.
	const FMouseButtonsArray& GetMouseButtons() const { return MouseButtonsDown; }

	// Get possibly empty range of indices bounding dirty part of the mouse buttons array.
	const FMouseButtonsIndexRange& GetMouseButtonsUpdateRange() const { return MouseButtonsUpdateRange; }

	// Change state of the button in the mouse buttons array and expand range bounding dirty part of the array.
	// @param MouseEvent - Mouse event representing mouse button
	// @param bIsDown - True, if button is down
	void SetMouseDown(const FPointerEvent& MouseEvent, bool bIsDown) { SetMouseDown(ImGuiInterops::GetMouseIndex(MouseEvent), bIsDown); }

	// Change state of the button in the mouse buttons array and expand range bounding dirty part of the array.
	// @param MouseButton - Mouse button key
	// @param bIsDown - True, if button is down
	void SetMouseDown(const FKey& MouseButton, bool bIsDown) { SetMouseDown(ImGuiInterops::GetMouseIndex(MouseButton), bIsDown); }

	// Get mouse wheel delta accumulated during the last frame.
	float GetMouseWheelDelta() const { return MouseWheelDelta; }

	// Add mouse wheel delta.
	// @param DeltaValue - Mouse wheel delta to add
	void AddMouseWheelDelta(float DeltaValue) { MouseWheelDelta += DeltaValue; }

	// Get the mouse position.
	const FVector2D& GetMousePosition() const { return MousePosition; }

	// Set the mouse position.
	// @param Position - Mouse position
	void SetMousePosition(const FVector2D& Position) { MousePosition = Position; }

	// Check whether input has active mouse pointer.
	bool HasMousePointer() const { return bHasMousePointer; }

	// Set whether input has active mouse pointer.
	// @param bHasPointer - True, if input has active mouse pointer
	void SetMousePointer(bool bInHasMousePointer) { bHasMousePointer = bInHasMousePointer; }

	// Check whether touch input is in progress. True, after touch is started until one frame after it has ended.
	// One frame delay is used to process mouse release in ImGui since touch-down is simulated with mouse-down.
	bool IsTouchActive() const { return bTouchDown || bTouchProcessed; }

	// Check whether touch input is down.
	bool IsTouchDown() const { return bTouchDown; }

	// Set whether touch input is down.
	// @param bIsDown - True, if touch is down (or started) and false, if touch is up (or ended)
	void SetTouchDown(bool bIsDown) { bTouchDown = bIsDown; }

	// Get the touch position.
	const FVector2D& GetTouchPosition() const { return TouchPosition; }

	// Set the touch position.
	// @param Position - Touch position
	void SetTouchPosition(const FVector2D& Position) { TouchPosition = Position; }

	// Get Control down state.
	bool IsControlDown() const { return bIsControlDown; }

	// Set Control down state.
	// @param bIsDown - True, if Control is down
	void SetControlDown(bool bIsDown) { bIsControlDown = bIsDown; }

	// Get Shift down state.
	bool IsShiftDown() const { return bIsShiftDown; }

	// Set Shift down state.
	// @param bIsDown - True, if Shift is down
	void SetShiftDown(bool bIsDown) { bIsShiftDown = bIsDown; }

	// Get Alt down state.
	bool IsAltDown() const { return bIsAltDown; }

	// Set Alt down state.
	// @param bIsDown - True, if Alt is down
	void SetAltDown(bool bIsDown) { bIsAltDown = bIsDown; }

	// Get reference to the array with navigation input states.
	const FNavInputArray& GetNavigationInputs() const { return NavigationInputs; }

	// Change state of the navigation input associated with this gamepad key.
	// @param KeyEvent - Key event with gamepad key input
	// @param bIsDown - True, if key is down
	void SetGamepadNavigationKey(const FKeyEvent& KeyEvent, bool bIsDown) { ImGuiInterops::SetGamepadNavigationKey(NavigationInputs, KeyEvent.GetKey(), bIsDown); }

	// Change state of the navigation input associated with this gamepad axis.
	// @param AnalogInputEvent - Analogue input event with gamepad axis input
	// @param Value - Analogue value that should be set for this axis
	void SetGamepadNavigationAxis(const FAnalogInputEvent& AnalogInputEvent, float Value) { ImGuiInterops::SetGamepadNavigationAxis(NavigationInputs, AnalogInputEvent.GetKey(), Value); }

	// Check whether keyboard navigation is enabled.
	bool IsKeyboardNavigationEnabled() const { return bKeyboardNavigationEnabled; }

	// Set whether keyboard navigation is enabled.
	// @param bEnabled - True, if navigation is enabled
	void SetKeyboardNavigationEnabled(bool bEnabled) { bKeyboardNavigationEnabled = bEnabled; }

	// Check whether gamepad navigation is enabled.
	bool IsGamepadNavigationEnabled() const { return bGamepadNavigationEnabled; }

	// Set whether gamepad navigation is enabled.
	// @param bEnabled - True, if navigation is enabled
	void SetGamepadNavigationEnabled(bool bEnabled) { bGamepadNavigationEnabled = bEnabled; }

	// Check whether gamepad is attached.
	bool HasGamepad() const { return bHasGamepad; }

	// Set whether gamepad is attached.
	// @param bInHasGamepad - True, if gamepad is attached
	void SetGamepad(bool bInHasGamepad) { bHasGamepad = bInHasGamepad; }

	// Reset the whole input state and mark it as dirty.
	void Reset()
	{
		ResetKeyboard();
		ResetMouse();
		ResetGamepadNavigation();
	}

	// Reset the keyboard input state and mark it as dirty.
	void ResetKeyboard()
	{
		ClearCharacters();
		ClearKeys();
		ClearModifierKeys();
	}

	// Reset the mouse input state and mark it as dirty.
	void ResetMouse()
	{
		ClearMouseButtons();
		ClearMouseAnalogue();
	}

	// Reset the gamepad navigation state.
	void ResetGamepadNavigation()
	{
		ClearNavigationInputs();
	}

	// Clear part of the state that is meant to be updated in every frame like: accumulators, buffers, navigation data
	// and information about dirty parts of keys or mouse buttons arrays.
	void ClearUpdateState();

	TMap<uint32, FKeyEvent> KeyDownEvents;
	TMap<uint32, FKeyEvent> KeyUpEvents;

private:

	void SetKeyDown(uint32 KeyIndex, bool bIsDown);
	void SetMouseDown(uint32 MouseIndex, bool IsDown);

	void ClearCharacters();
	void ClearKeys();
	void ClearMouseButtons();
	void ClearMouseAnalogue();
	void ClearModifierKeys();
	void ClearNavigationInputs();

	FVector2D MousePosition = FVector2D::ZeroVector;
	FVector2D TouchPosition = FVector2D::ZeroVector;
	float MouseWheelDelta = 0.f;

	FMouseButtonsArray MouseButtonsDown;
	FMouseButtonsIndexRange MouseButtonsUpdateRange;

	FCharactersBuffer InputCharacters;

	FKeysArray KeysDown;
	FKeysIndexRange KeysUpdateRange;

	FNavInputArray NavigationInputs;

	bool bHasMousePointer = false;
	bool bTouchDown = false;
	bool bTouchProcessed = false;

	bool bIsControlDown = false;
	bool bIsShiftDown = false;
	bool bIsAltDown = false;

	bool bKeyboardNavigationEnabled = false;
	bool bGamepadNavigationEnabled = false;
	bool bHasGamepad = false;
};
