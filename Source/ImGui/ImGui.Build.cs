// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

using System.IO;
using UnrealBuildTool;

public class ImGui : ModuleRules
{
	public ImGui(ReadOnlyTargetRules Target) : base(Target)
	{
		bool bBuildEditor = Target.bBuildEditor;

		// Developer modules are automatically loaded only in editor builds but can be stripped out from other builds.
		// Enable runtime loader, if you want this module to be automatically loaded in runtime builds (monolithic).
		bool bEnableRuntimeLoader = true;

		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;

		bLegacyPublicIncludePaths = false;
		CppCompileWarningSettings.ShadowVariableWarningLevel = WarningLevel.Error;

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "../ThirdParty/ImGuiLibrary/Include")
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "Private"),
				Path.Combine(ModuleDirectory, "../ThirdParty/ImGuiLibrary/Private")
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
				"CoreUObject",
				"Engine",
				"InputCore",
				"ApplicationCore",
				"RHI",
				"RenderCore",
				"Slate",
				"SlateCore"
				// ... add private dependencies that you statically link with here ...	
			}
			);


		if (bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Settings",
					"PropertyEditor",
					"UnrealEd",
				}
				);
		}


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);


		PrivateDefinitions.Add(string.Format("RUNTIME_LOADER_ENABLED={0}", bEnableRuntimeLoader ? 1 : 0));
	}
}
