// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

class FImGuiModuleManager;
class UGameViewportClient;
class UImGuiInputHandler;

class FImGuiInputHandlerFactory
{
public:

	static UImGuiInputHandler* NewHandler(const FStringClassReference& HandlerClassReference, FImGuiModuleManager* ModuleManager, UGameViewportClient* GameViewport, int32 ContextIndex);

	static void ReleaseHandler(UImGuiInputHandler* Handler);
};
