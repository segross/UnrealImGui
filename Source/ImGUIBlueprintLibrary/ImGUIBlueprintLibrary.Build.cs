// Some copyright should be here...

using UnrealBuildTool;

public class ImGUIBlueprintLibrary : ModuleRules
{
	public ImGUIBlueprintLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		bool RemoveOnShippingBuild = false;
		
		bEnforceIWYU = true;
		bUseUnity = false;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		if (RemoveOnShippingBuild)
		{
			if (Target.Configuration != UnrealTargetConfiguration.Shipping)
			{
				PrivateDependencyModuleNames.Add("ImGui");
			}
		}
		else
		{
			PrivateDependencyModuleNames.Add("ImGui");
		}

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
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
