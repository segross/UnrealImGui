// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiInputHandlerFactory.h"

#include "ImGuiInputHandler.h"
#include "ImGuiSettings.h"


DEFINE_LOG_CATEGORY_STATIC(LogImGuiInputHandlerFactory, Warning, All);

UImGuiInputHandler* FImGuiInputHandlerFactory::NewHandler(FImGuiModuleManager* ModuleManager, UGameViewportClient* GameViewport, int32 ContextIndex)
{
	UClass* HandlerClass = nullptr;
	if (UImGuiSettings* Settings = GetMutableDefault<UImGuiSettings>())
	{
		const auto& HandlerClassReference = Settings->GetImGuiInputHandlerClass();
		if (HandlerClassReference.IsValid())
		{
			HandlerClass = HandlerClassReference.TryLoadClass<UImGuiInputHandler>();

			if (!HandlerClass)
			{
				UE_LOG(LogImGuiInputHandlerFactory, Error, TEXT("Couldn't load ImGui Input Handler class '%s'."), *HandlerClassReference.ToString());
			}
		}
	}

	if (!HandlerClass)
	{
		HandlerClass = UImGuiInputHandler::StaticClass();
	}

	UImGuiInputHandler* Handler = NewObject<UImGuiInputHandler>(GameViewport, HandlerClass);
	if (Handler)
	{
		Handler->Initialize(ModuleManager, GameViewport, ContextIndex);
		Handler->AddToRoot();
	}
	else
	{
		UE_LOG(LogImGuiInputHandlerFactory, Error, TEXT("Failed attempt to create Input Handler: HandlerClass = %s."), *GetNameSafe(HandlerClass));
	}

	return Handler;
}

void FImGuiInputHandlerFactory::ReleaseHandler(UImGuiInputHandler* Handler)
{
	if (Handler)
	{
		Handler->RemoveFromRoot();
	}
}
