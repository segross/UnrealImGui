// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

class FImGuiModuleManager;
class UGameViewportClient;
class UImGuiInputHandler;

class FImGuiInputHandlerFactory
{
public:

	static UImGuiInputHandler* NewHandler(FImGuiModuleManager* ModuleManager, UGameViewportClient* GameViewport, int32 ContextIndex);

	static void ReleaseHandler(UImGuiInputHandler* Handler);
};
