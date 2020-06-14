// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <CoreMinimal.h>
#include <Input/Reply.h>
#include <UObject/Object.h>
#include <UObject/WeakObjectPtr.h>

#include "ImGuiInputHandler.generated.h"


class FImGuiModuleManager;
class UGameViewportClient;

struct FAnalogInputEvent;
struct FCharacterEvent;
struct FKeyEvent;

#if WITH_EDITOR
class FUICommandInfo;
#endif // WITH_EDITOR


/**
 * Handles input and sends it to the input state, which is copied to the ImGui IO at the beginning of the frame.
 * Implementation of the input handler can be changed in the ImGui project settings by changing ImGuiInputHandlerClass.
 */
UCLASS()
class IMGUI_API UImGuiInputHandler : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * Called to handle character events.
	 * @returns Response whether the event was handled
	 */
	virtual FReply OnKeyChar(const struct FCharacterEvent& CharacterEvent);

	/**
	 * Called to handle key down events.
	 * @returns Response whether the event was handled
	 */
	virtual FReply OnKeyDown(const FKeyEvent& KeyEvent);

	/**
	 * Called to handle key up events.
	 * @returns Response whether the event was handled
	 */
	virtual FReply OnKeyUp(const FKeyEvent& KeyEvent);

	/**
	 * Called to handle analog value change events.
	 * @returns Response whether the event was handled
	 */
	virtual FReply OnAnalogValueChanged(const FAnalogInputEvent& AnalogInputEvent);

	/**
	 * Called to handle mouse button down events.
	 * @returns Response whether the event was handled
	 */
	virtual FReply OnMouseButtonDown(const FPointerEvent& MouseEvent);

	/**
	 * Called to handle mouse button double-click events.
	 * @returns Response whether the event was handled
	 */
	virtual FReply OnMouseButtonDoubleClick(const FPointerEvent& MouseEvent);

	/**
	 * Called to handle mouse button up events.
	 * @returns Response whether the event was handled
	 */
	virtual FReply OnMouseButtonUp(const FPointerEvent& MouseEvent);

	/**
	 * Called to handle mouse wheel events.
	 * @returns Response whether the event was handled
	 */
	virtual FReply OnMouseWheel(const FPointerEvent& MouseEvent);

	/**
	 * Called to handle mouse move events.
	 * @param MousePosition Mouse position (in ImGui space)
	 * @param MouseEvent Optional mouse event passed from Slate
	 * @returns Response whether the event was handled
	 */
	virtual FReply OnMouseMove(const FVector2D& MousePosition, const FPointerEvent& MouseEvent);
	virtual FReply OnMouseMove(const FVector2D& MousePosition);

	/**
	 * Called to handle touch started event.
	 * @param TouchPosition Touch position (in ImGui space)
	 * @param TouchEvent Touch event passed from Slate
	 * @returns Response whether the event was handled
	 */
	virtual FReply OnTouchStarted(const FVector2D& TouchPosition, const FPointerEvent& TouchEvent);

	/**
	 * Called to handle touch moved event.
	 * @param TouchPosition Touch position (in ImGui space)
	 * @param TouchEvent Touch event passed from Slate
	 * @returns Response whether the event was handled
	 */
	virtual FReply OnTouchMoved(const FVector2D& TouchPosition, const FPointerEvent& TouchEvent);

	/**
	 * Called to handle touch ended event.
	 * @param TouchPosition Touch position (in ImGui space)
	 * @param TouchEvent Touch event passed from Slate
	 * @returns Response whether the event was handled
	 */
	virtual FReply OnTouchEnded(const FVector2D& TouchPosition, const FPointerEvent& TouchEvent);

	/** Called to handle activation of the keyboard input. */
	virtual void OnKeyboardInputEnabled();

	/** Called to handle deactivation of the keyboard input. */
	virtual void OnKeyboardInputDisabled();

	/** Called to handle activation of the gamepad input. */
	virtual void OnGamepadInputEnabled();

	/** Called to handle deactivation of the gamepad input. */
	virtual void OnGamepadInputDisabled();

	/** Called to handle activation of the mouse input. */
	virtual void OnMouseInputEnabled();

	/** Called to handle deactivation of the mouse input. */
	virtual void OnMouseInputDisabled();

protected:

	/** Copy state of modifier keys to input state. */
	void CopyModifierKeys(const FInputEvent& InputEvent);

	/**
	 * Checks whether this is a key event that can open console.
	 * @param KeyEvent - Key event to test.
	 * @returns True, if this key event can open console.
	 */
	bool IsConsoleEvent(const FKeyEvent& KeyEvent) const;

#if WITH_EDITOR
	/**
	 * Checks whether this is a key event that can stop PIE session.
	 * @param KeyEvent - Key event to test.
	 * @returns True, if this key event can stop PIE session.
	 */
	bool IsStopPlaySessionEvent(const FKeyEvent& KeyEvent) const;
#endif

	/**
	 * Checks whether this key event can toggle ImGui input (as defined in settings).
	 * @param KeyEvent - Key event to test.
	 * @returns True, if this key is bound to 'ImGui.ToggleInput' command that switches ImGui input mode.
	 */
	bool IsToggleInputEvent(const FKeyEvent& KeyEvent) const;

	/**
	 * Checks whether corresponding ImGui context has an active item (holding cursor focus).
	 * @returns True, if corresponding context has an active item.
	 */
	bool HasImGuiActiveItem() const;

private:

	void UpdateInputStatePointer();

	void OnSoftwareCursorChanged(bool);

	void OnPostImGuiUpdate();

	void Initialize(FImGuiModuleManager* InModuleManager, UGameViewportClient* InGameViewport, int32 InContextIndex);

	virtual void BeginDestroy() override;

	class FImGuiInputState* InputState = nullptr;

	bool bMouseInputEnabled = false;
	bool bKeyboardInputEnabled = false;
	bool bGamepadInputEnabled = false;

	FImGuiModuleManager* ModuleManager = nullptr;

	TWeakObjectPtr<UGameViewportClient> GameViewport;

	int32 ContextIndex = -1;

#if WITH_EDITOR
	TSharedPtr<FUICommandInfo> StopPlaySessionCommandInfo;
#endif

	friend class FImGuiInputHandlerFactory;
};
