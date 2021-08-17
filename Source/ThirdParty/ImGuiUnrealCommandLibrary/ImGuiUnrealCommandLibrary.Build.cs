using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

//=================================================================================================
// Source file to display a Dear ImGui Window with Unreal Commands support.
// This functionality comes from the UnrealNetImgui plugin.
// (Library can be found here : https://github.com/sammyfreg/UnrealNetImgui)
//
// Code is actually compiled under ImGuiModuleManager.cpp, avoiding issues with PCH and DLL
// functions definitions. This only adds some path/defines/modules dependencies
//=================================================================================================

public class ImGuiUnrealCommandLibrary : ModuleRules
{
#if WITH_FORWARDED_MODULE_RULES_CTOR
	public ImGuiUnrealCommandLibrary(ReadOnlyTargetRules Target) : base(Target)
#else
	public ImGuiUnrealCommandLibrary(TargetInfo Target)
#endif
	{
		//---------------------------------------------------------------------
		// Settings configuration 
		//---------------------------------------------------------------------
		bool kUseUnrealCommand		= true;	// Toggle UnrealCommand enabled here

		//---------------------------------------------------------------------
		// Setup Environment to build with/without UnrealCommand
		//---------------------------------------------------------------------
		Type = ModuleType.External;

#if !UE_4_19_OR_LATER
		List<string> PublicDefinitions = Definitions;
#endif
		if (kUseUnrealCommand)
		{
			PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Code"));
		}
		PublicDefinitions.Add(string.Format("IMGUI_UNREAL_COMMAND_ENABLED={0}", kUseUnrealCommand ? 1 : 0));
	}
}
