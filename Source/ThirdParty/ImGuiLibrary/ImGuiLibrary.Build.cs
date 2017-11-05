using UnrealBuildTool;

public class ImGuiLibrary : ModuleRules
{
#if WITH_FORWARDED_MODULE_RULES_CTOR
	public ImGuiLibrary(ReadOnlyTargetRules Target) : base(Target)
#else
	public ImGuiLibrary(TargetInfo Target)
#endif
	{
		Type = ModuleType.External;
	}
}
