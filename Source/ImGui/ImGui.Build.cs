// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

using System.IO;
using UnrealBuildTool;

public class ImGui : ModuleRules
{
	public ImGui(TargetInfo Target)
	{
		
		PublicIncludePaths.AddRange(
			new string[] {
				"ImGui/Public",
				Path.Combine(ModuleDirectory, "../ThirdParty/ImGuiLibrary/Include")
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"ImGui/Private",
				"ThirdParty/ImGuiLibrary/Private"
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Projects"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
