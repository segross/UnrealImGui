// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <ModuleManager.h>


class FImGuiModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
