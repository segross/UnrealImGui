// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VersionCompatibility.h"

#include <Curves/CurveFloat.h>
#include <Delegates/Delegate.h>
#include <InputCoreTypes.h>
#include <Styling/SlateTypes.h>
#include <UObject/Object.h>

// We use FSoftClassPath, which is supported by older and newer engine versions. Starting from 4.18, it is
// a typedef of FSoftClassPath, which is also recognized by UHT.
#if ENGINE_COMPATIBILITY_LEGACY_STRING_CLASS_REF
#include <StringClassReference.h>
#else
#include <UObject/SoftObjectPath.h>
#endif

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

UENUM(BlueprintType)
enum class EImGuiDPIScaleMethod : uint8
{
	ImGui UMETA(DisplayName = "ImGui", ToolTip = "Scale ImGui fonts and styles."),
	Slate UMETA(ToolTip = "Scale in Slate. ImGui canvas size will be adjusted to get the screen size that is the same as defined in the Canvas Size property.")
};

/**
 * Struct with DPI scale data.
 */
USTRUCT()
struct FImGuiDPIScaleInfo
{
	GENERATED_BODY()

protected:

	// Whether to scale in ImGui or in Slate. Scaling in ImGui gives better looking results but Slate might be a better
	// option when layouts do not account for different fonts and styles. When scaling in Slate, ImGui canvas size will
	// be adjusted to get the screen size that is the same as defined in the Canvas Size property.
	UPROPERTY(EditAnywhere, Category = "DPI Scale")
	EImGuiDPIScaleMethod ScalingMethod = EImGuiDPIScaleMethod::ImGui;

	// An optional scale to apply on top or instead of the curve-based scale.
	UPROPERTY(EditAnywhere, Category = "DPI Scale", meta = (ClampMin = 0, UIMin = 0))
	float Scale = 1.f;

	// Curve mapping resolution height to scale.
	UPROPERTY(config, EditAnywhere, Category = "DPI Scale", meta = (XAxisName = "Resolution Height", YAxisName = "Scale", EditCondition = "bScaleWithCurve"))
	FRuntimeFloatCurve DPICurve;

	// Whether to use curve-based scaling. If enabled, Scale will be multiplied by a value read from the DPICurve.
	// If disabled, only the Scale property will be used.
	UPROPERTY(config, EditAnywhere, Category = "DPI Scale")
	bool bScaleWithCurve = true;

public:

	FImGuiDPIScaleInfo();

	float GetImGuiScale() const { return ShouldScaleInSlate() ? 1.f : CalculateScale(); }

	float GetSlateScale() const { return ShouldScaleInSlate() ? CalculateScale() : 1.f; }

	bool ShouldScaleInSlate() const { return ScalingMethod == EImGuiDPIScaleMethod::Slate; }

private:

	float CalculateScale() const { return Scale * CalculateResolutionBasedScale(); }

	float CalculateResolutionBasedScale() const;
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
	FSoftClassPath ImGuiInputHandlerClass;

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

	// Setup DPI Scale.
	UPROPERTY(EditAnywhere, config, Category = "DPI Scale", Meta = (ShowOnlyInnerProperties))
	FImGuiDPIScaleInfo DPIScale;

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
	DECLARE_MULTICAST_DELEGATE_OneParam(FStringClassReferenceChangeDelegate, const FSoftClassPath&);
	DECLARE_MULTICAST_DELEGATE_OneParam(FImGuiCanvasSizeInfoChangeDelegate, const FImGuiCanvasSizeInfo&);
	DECLARE_MULTICAST_DELEGATE_OneParam(FImGuiDPIScaleInfoChangeDelegate, const FImGuiDPIScaleInfo&);

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
	const FSoftClassPath& GetImGuiInputHandlerClass() const { return ImGuiInputHandlerClass; }

	// Get the software cursor configuration.
	bool UseSoftwareCursor() const { return bUseSoftwareCursor; }

	// Get the shortcut configuration for 'ImGui.ToggleInput' command.
	const FImGuiKeyInfo& GetToggleInputKey() const { return ToggleInputKey; }

	// Get the information how to calculate the canvas size.
	const FImGuiCanvasSizeInfo& GetCanvasSizeInfo() const { return CanvasSize; }

	// Get the DPI Scale information.
	const FImGuiDPIScaleInfo& GetDPIScaleInfo() const { return DPIScale; }

	// Delegate raised when ImGui Input Handle is changed.
	FStringClassReferenceChangeDelegate OnImGuiInputHandlerClassChanged;

	// Delegate raised when software cursor configuration is changed.
	FBoolChangeDelegate OnUseSoftwareCursorChanged;

	// Delegate raised when information how to calculate the canvas size is changed.
	FImGuiCanvasSizeInfoChangeDelegate OnCanvasSizeChangedDelegate;

	// Delegate raised when the DPI scale is changed.
	FImGuiDPIScaleInfoChangeDelegate OnDPIScaleChangedDelegate;

private:

	void InitializeAllSettings();
	void UpdateSettings();
	void UpdateDPIScaleInfo();

	void SetImGuiInputHandlerClass(const FSoftClassPath& ClassReference);
	void SetShareKeyboardInput(bool bShare);
	void SetShareGamepadInput(bool bShare);
	void SetShareMouseInput(bool bShare);
	void SetUseSoftwareCursor(bool bUse);
	void SetToggleInputKey(const FImGuiKeyInfo& KeyInfo);
	void SetCanvasSizeInfo(const FImGuiCanvasSizeInfo& CanvasSizeInfo);
	void SetDPIScaleInfo(const FImGuiDPIScaleInfo& ScaleInfo);

#if WITH_EDITOR
	void OnPropertyChanged(class UObject* ObjectBeingModified, struct FPropertyChangedEvent& PropertyChangedEvent);
#endif // WITH_EDITOR

	FImGuiModuleProperties& Properties;
	FImGuiModuleCommands& Commands;

	FSoftClassPath ImGuiInputHandlerClass;
	FImGuiKeyInfo ToggleInputKey;
	FImGuiCanvasSizeInfo CanvasSize;
	FImGuiDPIScaleInfo DPIScale;
	bool bShareKeyboardInput = false;
	bool bShareGamepadInput = false;
	bool bShareMouseInput = false;
	bool bUseSoftwareCursor = false;
};
