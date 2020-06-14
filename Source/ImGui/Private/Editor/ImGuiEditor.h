// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include <Delegates/IDelegateInstance.h>


// Registers module's settings in editor (due to a small size of this code we don't use a separate editor module).
class FImGuiEditor
{
public:

	FImGuiEditor();
	~FImGuiEditor();

private:

	bool IsRegistrationCompleted() const { return bSettingsRegistered && bCustomPropertyTypeLayoutsRegistered; }

	void Register();
	void Unregister();

	void CreateRegistrator();
	void ReleaseRegistrator();

	FDelegateHandle RegistratorHandle;

	bool bSettingsRegistered = false;
	bool bCustomPropertyTypeLayoutsRegistered = false;
};

#endif // WITH_EDITOR
