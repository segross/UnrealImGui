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

	// Delegate raised when ImGuiInputHandlerClass property has changed.
	FSimpleMulticastDelegate OnImGuiInputHandlerClassChanged;

protected:

	// Path to own implementation of ImGui Input Handler allowing to customize handling of keyboard and gamepad input.
	// If not set then default handler is used.
	UPROPERTY(EditAnywhere, config, Category = "Input", meta = (MetaClass = "ImGuiInputHandler"))
	FStringClassReference ImGuiInputHandlerClass;

private:

#if WITH_EDITOR

	void RegisterPropertyChangedDelegate();
	void UnregisterPropertyChangedDelegate();

	void OnPropertyChanged(class UObject* ObjectBeingModified, struct FPropertyChangedEvent& PropertyChangedEvent);

#endif // WITH_EDITOR
};
