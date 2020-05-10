// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include <Delegates/Delegate.h>
#include <UObject/Object.h>

#include "ImGuiModuleSettings.generated.h"


/**
 * Struct containing key information that can be used for key binding. Using 'Undetermined' value for modifier keys
 * means that those keys should be ignored when testing for a match.
 */
USTRUCT()
struct FImGuiKeyInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	FKey Key;

	UPROPERTY(EditAnywhere, Category = "Input")
	ECheckBoxState Shift = ECheckBoxState::Undetermined;

	UPROPERTY(EditAnywhere, Category = "Input")
	ECheckBoxState Ctrl = ECheckBoxState::Undetermined;

	UPROPERTY(EditAnywhere, Category = "Input")
	ECheckBoxState Alt = ECheckBoxState::Undetermined;

	UPROPERTY(EditAnywhere, Category = "Input")
	ECheckBoxState Cmd = ECheckBoxState::Undetermined;

	friend bool operator==(const FImGuiKeyInfo& Lhs, const FImGuiKeyInfo& Rhs)
	{
		return Lhs.Key == Rhs.Key
			&& Lhs.Shift == Rhs.Shift
			&& Lhs.Ctrl == Rhs.Ctrl
			&& Lhs.Alt == Rhs.Alt
			&& Lhs.Cmd == Rhs.Cmd;
	}

	friend bool operator!=(const FImGuiKeyInfo& Lhs, const FImGuiKeyInfo& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

UENUM(BlueprintType)
enum class EImGuiCanvasSizeType : uint8
{
	Custom UMETA(ToolTip = "Canvas will have a custom width and height."),
	Desktop UMETA(ToolTip = "Canvas will have the same width and height as the desktop."),
	Viewport UMETA(ToolTip = "Canvas will always have the same width and height as the viewport."),
};

/**
 * Struct with information how to calculate canvas size. 
 */
USTRUCT()
struct FImGuiCanvasSizeInfo
{
	GENERATED_BODY()

	// Select how to specify canvas size.
	UPROPERTY(EditAnywhere, Category = "Canvas Size")
	EImGuiCanvasSizeType SizeType = EImGuiCanvasSizeType::Desktop;

	// Custom canvas width.
	UPROPERTY(EditAnywhere, Category = "Canvas Size", meta = (ClampMin = 0, UIMin = 0))
	int32 Width = 3840;

	// Custom canvas height.
	UPROPERTY(EditAnywhere, Category = "Canvas Size", meta = (ClampMin = 0, UIMin = 0))
	int32 Height = 2160;

	// If this is true, canvas width or height may be extended, if the viewport size is larger.
	UPROPERTY(EditAnywhere, Category = "Canvas Size", meta = (ClampMin = 0, UIMin = 0))
	bool bExtendToViewport = true;

	bool operator==(const FImGuiCanvasSizeInfo& Other) const
	{
		return (SizeType == Other.SizeType) && (Width == Other.Width)
			&& (Height == Other.Height) && (bExtendToViewport == Other.bExtendToViewport);
	}

	bool operator!=(const FImGuiCanvasSizeInfo& Other) const { return !(*this == Other); }
};

// UObject used for loading and saving ImGui settings. To access actual settings use FImGuiModuleSettings interface.
UCLASS(config=ImGui, defaultconfig)
class UImGuiSettings : public UObject
{
	GENERATED_BODY()

public:

	// Get default instance or null if it is not loaded.
	static UImGuiSettings* Get() { return DefaultInstance; }

	// Delegate raised when default instance is loaded.
	static FSimpleMulticastDelegate OnSettingsLoaded;

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

	// Whether ImGui should share mouse input with game.
	// This defines initial behaviour which can be later changed using 'ImGui.ToggleMouseInputSharing' command or
	// module properties interface.
	UPROPERTY(EditAnywhere, config, Category = "Input")
	bool bShareMouseInput = false;

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

	// Chose how to define the ImGui canvas size. Select between custom, desktop and viewport.
	UPROPERTY(EditAnywhere, config, Category = "Canvas Size")
	FImGuiCanvasSizeInfo CanvasSize;

	// Deprecated name for ToggleInput. Kept temporarily to automatically move old configuration.
	UPROPERTY(config)
	FImGuiKeyInfo SwitchInputModeKey_DEPRECATED;

	static UImGuiSettings* DefaultInstance;

	friend class FImGuiModuleSettings;
};


class FImGuiModuleCommands;
class FImGuiModuleProperties;

// Interface for ImGui module settings. It shadows all the settings and keep them in sync after UImGuiSettings class is
// loaded, but it can also work before that time what simplifies workflow in early-loading scenarios.
// It binds to module properties and commands objects that need to be passed during construction.
class FImGuiModuleSettings
{
public:

	// Generic delegate used to notify changes of boolean properties.
	DECLARE_MULTICAST_DELEGATE_OneParam(FBoolChangeDelegate, bool);
	DECLARE_MULTICAST_DELEGATE_OneParam(FStringClassReferenceChangeDelegate, const FStringClassReference&);

	// Constructor for ImGui module settings. It will bind to instances of module properties and commands and will
	// update them every time when settings are changed.
	//
	// @param InProperties - Instance of module properties that will be bound and updated by this object.
	// @param InCommands - Instance of module commands that will be bound and updated by this object.
	FImGuiModuleSettings(FImGuiModuleProperties& InProperties, FImGuiModuleCommands& InCommands);
	~FImGuiModuleSettings();

	// It doesn't offer interface for settings that define initial values for properties, as those are passed during
	// start-up and should be accessed trough properties interface. Remaining settings can have getter and/or change
	// event that are defined depending on needs.

	// Get the path to custom implementation of ImGui Input Handler.
	const FStringClassReference& GetImGuiInputHandlerClass() const { return ImGuiInputHandlerClass; }

	// Get the software cursor configuration.
	bool UseSoftwareCursor() const { return bUseSoftwareCursor; }

	// Get the shortcut configuration for 'ImGui.ToggleInput' command.
	const FImGuiKeyInfo& GetToggleInputKey() const { return ToggleInputKey; }

	// Get the information how to calculate the canvas size.
	const FImGuiCanvasSizeInfo& GetCanvasSizeInfo() const { return CanvasSize; }

	// Delegate raised when ImGui Input Handle is changed.
	FStringClassReferenceChangeDelegate OnImGuiInputHandlerClassChanged;

	// Delegate raised when software cursor configuration is changed.
	FBoolChangeDelegate OnUseSoftwareCursorChanged;

private:

	void UpdateSettings();

	void SetImGuiInputHandlerClass(const FStringClassReference& ClassReference);
	void SetShareKeyboardInput(bool bShare);
	void SetShareGamepadInput(bool bShare);
	void SetShareMouseInput(bool bShare);
	void SetUseSoftwareCursor(bool bUse);
	void SetToggleInputKey(const FImGuiKeyInfo& KeyInfo);
	void SetCanvasSizeInfo(const FImGuiCanvasSizeInfo& CanvasSizeInfo);

#if WITH_EDITOR
	void OnPropertyChanged(class UObject* ObjectBeingModified, struct FPropertyChangedEvent& PropertyChangedEvent);
#endif // WITH_EDITOR

	FImGuiModuleProperties& Properties;
	FImGuiModuleCommands& Commands;

	FStringClassReference ImGuiInputHandlerClass;
	FImGuiKeyInfo ToggleInputKey;
	FImGuiCanvasSizeInfo CanvasSize;
	bool bShareKeyboardInput = false;
	bool bShareGamepadInput = false;
	bool bShareMouseInput = false;
	bool bUseSoftwareCursor = false;
};
