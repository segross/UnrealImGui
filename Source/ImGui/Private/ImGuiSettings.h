// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ImGuiInputHandler.h"

#include <Delegates/Delegate.h>
#include <UObject/Object.h>

// Select right soft class reference header to avoid warning.
#if ENGINE_COMPATIBILITY_LEGACY_STRING_CLASS_REF
#include <StringClassReference.h>
#else
#include <UObject/SoftObjectPath.h>
#endif

#include "ImGuiSettings.generated.h"


/**
 * Struct containing key information that can be used for key binding. Using 'Undetermined' value for modifier keys
 * means that those keys should be ignored when testing for a match.
 */
USTRUCT()
struct FImGuiKeyInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FKey Key;

	UPROPERTY(EditAnywhere)
	ECheckBoxState Shift = ECheckBoxState::Undetermined;

	UPROPERTY(EditAnywhere)
	ECheckBoxState Ctrl = ECheckBoxState::Undetermined;

	UPROPERTY(EditAnywhere)
	ECheckBoxState Alt = ECheckBoxState::Undetermined;

	UPROPERTY(EditAnywhere)
	ECheckBoxState Cmd = ECheckBoxState::Undetermined;
};

// Settings for ImGui module.
UCLASS(config=ImGui, defaultconfig)
class UImGuiSettings : public UObject
{
	GENERATED_BODY()

public:

	// Generic delegate used to notify changes of boolean properties.
	DECLARE_MULTICAST_DELEGATE_OneParam(FBoolChangeDelegate, bool);

	// Delegate raised when settings instance is loaded.
	static FSimpleMulticastDelegate& OnSettingsLoaded();

	UImGuiSettings();
	~UImGuiSettings();

	// Get the path to custom implementation of ImGui Input Handler.
	const FStringClassReference& GetImGuiInputHandlerClass() const { return ImGuiInputHandlerClass; }

	// Get the keyboard input sharing configuration.
	bool ShareKeyboardInput() const { return bShareKeyboardInput; }

	// Get the gamepad input sharing configuration.
	bool ShareGamepadInput() const { return bShareGamepadInput; }

	// Get the software cursor configuration.
	bool UseSoftwareCursor() const { return bUseSoftwareCursor; }

	// Get the shortcut configuration for 'ImGui.ToggleInput' command.
	const FImGuiKeyInfo& GetToggleInputKey() const { return ToggleInput; }

	// Delegate raised when ImGui Input Handle is changed.
	FSimpleMulticastDelegate OnImGuiInputHandlerClassChanged;

	// Delegate raised when keyboard input sharing configuration is changed.
	FBoolChangeDelegate OnShareKeyboardInputChanged;

	// Delegate raised when gamepad input sharing configuration is changed.
	FBoolChangeDelegate OnShareGamepadInputChanged;

	// Delegate raised when software cursor configuration is changed.
	FSimpleMulticastDelegate OnSoftwareCursorChanged;

	// Delegate raised when shortcut configuration for 'ImGui.ToggleInput' command is changed.
	FSimpleMulticastDelegate OnToggleInputKeyChanged;

	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

protected:

	// Path to own implementation of ImGui Input Handler allowing to customize handling of keyboard and gamepad input.
	// If not set then default handler is used.
	UPROPERTY(EditAnywhere, config, Category = "Extensions", meta = (MetaClass = "ImGuiInputHandler"))
	FStringClassReference ImGuiInputHandlerClass;

	// Whether ImGui should share keyboard input with game.
	// This defines initial behaviour which can be later changed using 'ImGui.ToggleKeyboardInputSharing' command or
	// module properties interface.
	UPROPERTY(EditAnywhere, config, Category = "Input")
	bool bShareKeyboardInput = false;

	// Whether ImGui should share gamepad input with game.
	// This defines initial behaviour which can be later changed using 'ImGui.ToggleGamepadInputSharing' command or
	// module properties interface.
	UPROPERTY(EditAnywhere, config, Category = "Input")
	bool bShareGamepadInput = false;

	// If true, then in input mode ImGui will draw its own cursor in place of the hardware one.
	// When disabled (default) there is a noticeable difference between cursor position seen by ImGui and position on
	// the screen. Enabling this option removes that effect but with lower frame-rates UI becomes quickly unusable.
	UPROPERTY(EditAnywhere, config, Category = "Input", AdvancedDisplay)
	bool bUseSoftwareCursor = false;

	// Define a shortcut key to 'ImGui.ToggleInput' command. Binding is only set if the key field is valid.
	// Note that modifier key properties can be set to one of the three values: undetermined means that state of the given
	// modifier is not important, checked means that it needs to be pressed and unchecked means that it cannot be pressed.
	//
	// This binding is using Player Input's DebugExecBindings which only works in non-shipment builds.
	UPROPERTY(EditAnywhere, config, Category = "Keyboard Shortcuts")
	FImGuiKeyInfo ToggleInput;

	// Deprecated name for ToggleInput. Kept temporarily to automatically move old configuration.
	UPROPERTY(config)
	FImGuiKeyInfo SwitchInputModeKey_DEPRECATED;

private:

#if WITH_EDITOR

	void RegisterPropertyChangedDelegate();
	void UnregisterPropertyChangedDelegate();

	void OnPropertyChanged(class UObject* ObjectBeingModified, struct FPropertyChangedEvent& PropertyChangedEvent);

#endif // WITH_EDITOR
};

// Pointer to the settings instance (default class object) assigned right after it is initialized and valid until
// it is destroyed.
extern UImGuiSettings* GImGuiSettings;
