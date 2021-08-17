// Distributed under the MIT License (MIT) (see accompanying LICENSE file)
//
// @author	: Sammy Fatnassi
// @date	: 2021/08/08
// @version : 0.1
// @brief	: Support for 'Unreal Commands' through 'Dear ImGui UI'.
// @note	: Used 'SOutputLog.cpp' and 'FConsoleCommandExecutor.cpp' as reference
// @note	: This '.h/.cpp pair' is part of the 'UnrealNetImgui' library, but can be used standalone in your own Dear ImGui Unreal integration
// @usage	: Search for 'IMGUI_UNREAL_COMMAND_ENABLED' in 'https://github.com/sammyfreg/UnrealNetImgui/blob/master/Source/Private/NetImguiModule.cpp'
//			1- Call 'Create()' once
//			2- Toggle visibility by changing 'IsVisible()' returned value
//			3- Call 'Show()' every frame
//			4- Call 'Destroy()' once, when program is closing
#pragma once

#ifndef IMGUI_UNREAL_COMMAND_ENABLED
#define IMGUI_UNREAL_COMMAND_ENABLED 1 // Default value is 'Active'. Define to 0 to remove 'Dear ImGui Unreal Command' support
#endif

#if IMGUI_UNREAL_COMMAND_ENABLED

#include "CoreMinimal.h"

namespace UECommandImgui
{
	struct			CommandContext;

	// @brief		Initialize the Context
	// @oaram		[addDefaultPresets] Fill the Preset list, with commonly used value
	CommandContext*	Create(bool addDefaultPresets = true);
	
	// @brief		Release the created Context
	void			Destroy(CommandContext*& pCmdContext);
	
	// @brief		Display the 'UE Command' Window, using Dear ImGui
	void			Show(CommandContext* pCmdContext);
	
	// @brief		True if the Window should be displayed
	// @note		Returned value can be modified to change the Window visibility
	bool&			IsVisible(CommandContext* pCmdContext);
	
	// @brief		Add new 'Commands' to the specified Preset commands list. 
	//				Commanda are executed as is
	void			AddPresetCommands(CommandContext* pCmdContext, const FString& presetName, const TArray<FString>& commands);

	// @brief		Add new 'Filters' to the specified Preset commands list. 
	//				Filters are used to find all 'Unreal Commands' starting with this string, and add them to the Preset
	void			AddPresetFilters(CommandContext* pCmdContext, const FString& presetName, const TArray<FString>& filters);
}

#endif // IMGUI_UNREAL_COMMAND_ENABLED
