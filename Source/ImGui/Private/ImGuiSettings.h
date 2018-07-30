// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ImGuiInputHandler.h"

#include <Delegates/Delegate.h>
#include <UObject/Object.h>

// Select right soft class reference header to avoid warning (new header contains FSoftClassPath to FStringClassReference
// typedef, so we will use that as a common denominator).
#include <Runtime/Launch/Resources/Version.h>
#if (ENGINE_MAJOR_VERSION < 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 18))
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

	UImGuiSettings();
	~UImGuiSettings();

	// Path to custom implementation of ImGui Input Handler.
	const FStringClassReference& GetImGuiInputHandlerClass() const { return ImGuiInputHandlerClass; }

	// Get mapping for 'ImGui.SwitchInputMode' command.
	const FImGuiKeyInfo& GetSwitchInputModeKey() const { return SwitchInputModeKey; }

	// Delegate raised when ImGuiInputHandlerClass property has changed.
	FSimpleMulticastDelegate OnImGuiInputHandlerClassChanged;

	virtual void PostInitProperties() override;

protected:

	// Path to own implementation of ImGui Input Handler allowing to customize handling of keyboard and gamepad input.
	// If not set then default handler is used.
	UPROPERTY(EditAnywhere, config, Category = "Input", meta = (MetaClass = "ImGuiInputHandler"))
	FStringClassReference ImGuiInputHandlerClass;

	// Define a custom key binding to 'ImGui.SwitchInputMode' command. Mapping will be only set if Key property in this
	// structure is set to a valid key. Modifier keys can be either completely ignored (Undetermined), required to be
	// pressed (Checked) or required to be not pressed (Unchecked).
	UPROPERTY(EditAnywhere, config, Category = "Input")
	FImGuiKeyInfo SwitchInputModeKey;

private:

	void UpdateSwitchInputModeBinding();

#if WITH_EDITOR

	void RegisterPropertyChangedDelegate();
	void UnregisterPropertyChangedDelegate();

	void OnPropertyChanged(class UObject* ObjectBeingModified, struct FPropertyChangedEvent& PropertyChangedEvent);

#endif // WITH_EDITOR
};
