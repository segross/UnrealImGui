// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiModuleSettings.h"


class FImGuiModuleManager;
class UGameViewportClient;
class UImGuiInputHandler;

class FImGuiInputHandlerFactory
{
public:

	static UImGuiInputHandler* NewHandler(const FSoftClassPath& HandlerClassReference, FImGuiModuleManager* ModuleManager, UObject* Outer, FName ContextIndex);
	static void ReleaseHandler(UImGuiInputHandler* Handler);
};
