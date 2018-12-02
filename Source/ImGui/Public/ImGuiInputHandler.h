// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <CoreMinimal.h>
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


/** Response used by ImGui Input Handler to communicate input handling requests. */
struct IMGUI_API FImGuiInputResponse
{
	/** Create empty response with no requests. */
	FImGuiInputResponse() = default;

	/**
	 * Create response with custom request configuration.
	 *
	 * @param bInProcess - State of the processing request.
	 * @param bInConsume - State of the consume request.
	 */
	FImGuiInputResponse(bool bInProcess, bool bInConsume)
		: bProcess(bInProcess)
		, bConsume(bInConsume)
	{}

	/**
	 * Check whether this response contains processing request.
	 *
	 * @returns True, if processing was requested and false otherwise.
	 */
	FORCEINLINE bool HasProcessingRequest() const { return bProcess; }

	/**
	 * Check whether this response contains consume request.
	 *
	 * @returns True, if consume was requested and false otherwise.
	 */
	FORCEINLINE bool HasConsumeRequest() const { return bConsume; }

	/**
	 * Set the processing request.
	 *
	 * @param bInProcess - True, to request input processing (implicit) and false otherwise.
	 * @returns Reference to this response (for chaining requests).
	 */
	FORCEINLINE FImGuiInputResponse& RequestProcessing(bool bInProcess = true) { bProcess = bInProcess; return *this; }

	/**
	 * Set the consume request.
	 *
	 * @param bInConsume - True, to request input consume (implicit) and false otherwise.
	 * @returns Reference to this response (for chaining requests).
	 */
	FORCEINLINE FImGuiInputResponse& RequestConsume(bool bInConsume = true) { bConsume = bInConsume; return *this; }

private:

	bool bProcess = false;

	bool bConsume = false;
};

/**
 * Defines behaviour when handling input events. It allows to customize handling of the keyboard and gamepad input,
 * primarily to support shortcuts in ImGui input mode. Since mouse is not really needed for this functionality and
 * mouse pointer state and focus are closely connected to input mode, mouse events are left out of this interface.
 *
 * When receiving keyboard and gamepad events ImGui Widget calls input handler to query expected behaviour. By default,
 * with a few exceptions (see @ OnKeyDown) all events are expected to be processed and consumed. Custom implementations
 * may tweak that behaviour and/or inject custom code.
 *
 * Note that returned response is only treated as a hint. In current implementation all consume requests are respected
 * but to protect from locking ImGui input states, key up events are always processed. Decision about blocking certain
 * inputs can be taken during key down events and processing corresponding key up events should not make difference.
 *
 * Also note that input handler functions are only called when ImGui Widget is receiving input events, what can be for
 * instance suppressed by opening console.
 *
 * See @ Project Settings/Plugins/ImGui/Extensions/ImGuiInputHandlerClass property to set custom implementation.
 */
UCLASS()
class IMGUI_API UImGuiInputHandler : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * Called when handling character events.
	 *
	 * @returns Response with rules how input should be handled. Default implementation contains requests to process
	 * and consume this event.
	 */
	virtual FImGuiInputResponse OnKeyChar(const struct FCharacterEvent& CharacterEvent) { return GetDefaultKeyboardResponse(); }

	/**
	 * Called when handling keyboard key down events.
	 *
	 * @returns Response with rules how input should be handled. Default implementation contains requests to process
	 * and consume most of the key, but unlike other cases it requests to ignore certain events, like those that are
	 * needed to open console or close PIE session in editor.
	 */
	virtual FImGuiInputResponse OnKeyDown(const FKeyEvent& KeyEvent);

	/**
	 * Called when handling keyboard key up events.
	 *
	 * Note that regardless of returned response, key up events are always processed by ImGui Widget.
	 *
	 * @returns Response with rules how input should be handled. Default implementation contains requests to consume
	 * this event.
	 */
	virtual FImGuiInputResponse OnKeyUp(const FKeyEvent& KeyEvent) { return GetDefaultKeyboardResponse(); }

	/**
	 * Called when handling gamepad key down events.
	 *
	 * @returns Response with rules how input should be handled. Default implementation contains requests to process
	 * and consume this event.
	 */
	virtual FImGuiInputResponse OnGamepadKeyDown(const FKeyEvent& GamepadKeyEvent) { return GetDefaultGamepadResponse(); }

	/**
	 * Called when handling gamepad key up events.
	 *
	 * Note that regardless of returned response, key up events are always processed by ImGui Widget.
	 *
	 * @returns Response with rules how input should be handled. Default implementation contains requests to consume
	 * this event.
	 */
	virtual FImGuiInputResponse OnGamepadKeyUp(const FKeyEvent& GamepadKeyEvent) { return GetDefaultGamepadResponse(); }

	/**
	 * Called when handling gamepad analog events.
	 *
	 * @returns Response with rules how input should be handled. Default implementation contains requests to process
	 * and consume this event.
	 */
	virtual FImGuiInputResponse OnGamepadAxis(const FAnalogInputEvent& GamepadAxisEvent) { return GetDefaultGamepadResponse(); }

protected:

	/**
	 * Get default keyboard response, with consume request based on IsKeyboardInputShared property.
	 *
	 * @returns Default response for keyboard inputs.
	 */
	FImGuiInputResponse GetDefaultKeyboardResponse() const;

	/**
	 * Get default gamepad response, with consume request based on IsGamepadInputShared property.
	 *
	 * @returns Default response for gamepad inputs.
	 */
	FImGuiInputResponse GetDefaultGamepadResponse() const;

	/**
	 * Checks whether this is a key event that can open console.
	 *
	 * @param KeyEvent - Key event to test.
	 * @returns True, if this key event can open console.
	 */
	bool IsConsoleEvent(const FKeyEvent& KeyEvent) const;

#if WITH_EDITOR
	/**
	 * Checks whether this is a key event that can stop PIE session.
	 *
	 * @param KeyEvent - Key event to test.
	 * @returns True, if this key event can stop PIE session.
	 */
	bool IsStopPlaySessionEvent(const FKeyEvent& KeyEvent) const;
#endif

	/**
	 * Checks whether this key event can toggle ImGui input (as defined in settings).
	 *
	 * @param KeyEvent - Key event to test.
	 * @returns True, if this key is bound to 'ImGui.ToggleInput' command that switches ImGui input mode.
	 */
	bool IsToggleInputEvent(const FKeyEvent& KeyEvent) const;

	/**
	 * Checks whether corresponding ImGui context has an active item (holding cursor focus).
	 *
	 * @returns True, if corresponding context has an active item.
	 */
	bool HasImGuiActiveItem() const;

private:

	void Initialize(FImGuiModuleManager* InModuleManager, UGameViewportClient* InGameViewport, int32 InContextIndex);

	FImGuiModuleManager* ModuleManager = nullptr;

	TWeakObjectPtr<UGameViewportClient> GameViewport;

	int32 ContextIndex = -1;

#if WITH_EDITOR
	TSharedPtr<FUICommandInfo> StopPlaySessionCommandInfo;
#endif

	friend class FImGuiInputHandlerFactory;
};
