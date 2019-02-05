// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#if UE_4_18_OR_LATER
#define WITH_POST_ACTOR_TICK
#endif

using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

public class ImGui : ModuleRules
{
	// Defines when events should be broadcast. Note that at the end of the ImGui frame some global variables might be
	// not set and so it seems preferable to use post actor tick (not available in older engine versions) or world tick
	// start events. If more control is required Late mode with manually calling events may be used (as in practice
	// events are guaranteed to be raised only once per frame).
	enum EEventsBroadcastMode
	{
		OnWorldTickStart,   // Broadcast during world tick start event.
#if WITH_POST_ACTOR_TICK
		OnPostActorTick,    // Broadcast during post actor tick event.
#endif
		Late,               // Broadcast at the end of the ImGui frame.
	}

	// Defines order in which multi-context and world-context events are called.
	enum EEventsOrder
	{
		MultiBeforeWorldContext,
		WorldBeforeMultiContext,
	}

#if WITH_POST_ACTOR_TICK
	EEventsBroadcastMode DrawEventsBroadcastMode = EEventsBroadcastMode.OnPostActorTick;
	EEventsOrder DrawEventsOrder = EEventsOrder.WorldBeforeMultiContext;
#else
	EEventsBroadcastMode DrawEventsBroadcastMode = EEventsBroadcastMode.OnWorldTickStart;
	EEventsOrder DrawEventsOrder = EEventsOrder.MultiBeforeWorldContext;
#endif // WITH_POST_ACTOR_TICK



#if WITH_FORWARDED_MODULE_RULES_CTOR
	public ImGui(ReadOnlyTargetRules Target) : base(Target)
#else
	public ImGui(TargetInfo Target)
#endif
	{

#if WITH_FORWARDED_MODULE_RULES_CTOR
		bool bBuildEditor = Target.bBuildEditor;
#else
		bool bBuildEditor = (Target.Type == TargetRules.TargetType.Editor);
#endif

		// Developer modules are automatically loaded only in editor builds but can be stripped out from other builds.
		// Enable runtime loader, if you want this module to be automatically loaded in runtime builds (monolithic).
		bool bEnableRuntimeLoader = true;

#if UE_4_21_OR_LATER
		PrivatePCHHeaderFile = "Private/ImGuiPrivatePCH.h";
#endif

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "../ThirdParty/ImGuiLibrary/Include"),
				Path.Combine(ModuleDirectory, "../ThirdParty/ImGuiLibrary/Include/misc/stl")
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
				"CoreUObject",
				"Engine",
				"InputCore",
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
					"EditorStyle",
					"Settings",
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


#if !UE_4_19_OR_LATER
		List<string> PrivateDefinitions = Definitions;
#endif

		PrivateDefinitions.Add(string.Format("RUNTIME_LOADER_ENABLED={0}", bEnableRuntimeLoader ? 1 : 0));

		bool bDrawOnWorldTickStart = (DrawEventsBroadcastMode == EEventsBroadcastMode.OnWorldTickStart);
#if WITH_POST_ACTOR_TICK
		bool bDrawOnPostActorTick = (DrawEventsBroadcastMode == EEventsBroadcastMode.OnPostActorTick);
#else
		bool bDrawOnPostActorTick = false;
#endif

		PrivateDefinitions.Add(string.Format("DRAW_EVENTS_ON_WORLD_TICK_START={0}", bDrawOnWorldTickStart ? 1 : 0));
		PrivateDefinitions.Add(string.Format("DRAW_EVENTS_ON_POST_ACTOR_TICK={0}", bDrawOnPostActorTick ? 1 : 0));
		PrivateDefinitions.Add(string.Format("DRAW_EVENTS_ORDER_WORLD_BEFORE_MULTI_CONTEXT={0}",
			(DrawEventsOrder == EEventsOrder.WorldBeforeMultiContext) ? 1 : 0));
	}
}
