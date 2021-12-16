using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

//=================================================================================================
// netImgui Build Setup
// (Library can be found here : https://github.com/sammyfreg/netImgui)
//
// Code is actually compiled under ThirdPartyBuildNetImgui.cpp, avoiding issues with PCH and DLL
// functions definitions. This only some path/defines/modules dependencies
//=================================================================================================

public class NetImguiLibrary : ModuleRules
{
#if WITH_FORWARDED_MODULE_RULES_CTOR
	public NetImguiLibrary(ReadOnlyTargetRules Target) : base(Target)
#else
	public NetImguiLibrary(TargetInfo Target)
#endif
	{
		//---------------------------------------------------------------------
		// Settings configuration 
		//---------------------------------------------------------------------
		bool kUseNetImgui			= true;									// Toggle netImgui enabled here
		string kGameListenPort		= "(NetImgui::kDefaultClientPort)";		// Com Port used by Game exe to wait for a connection from netImgui Server (8889 by default)
		string kEditorListenPort	= "(NetImgui::kDefaultClientPort+1)";   // Com Port used by Editor exe to wait for a connection from netImgui Server (8890 by default)
		string kServerListenPort	= "(NetImgui::kDefaultClientPort+2)";   // Com Port used by Server exe to wait for a connection from netImgui Server (8891 by default)

		//---------------------------------------------------------------------
		// Setup Environment to build with/without netImgui
		//---------------------------------------------------------------------
		Type = ModuleType.External;

#if !UE_4_19_OR_LATER
		List<string> PublicDefinitions = Definitions;
#endif
		if( kUseNetImgui )
		{
			PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Client"));
			PublicDependencyModuleNames.Add("Sockets");

			PublicDefinitions.Add("NETIMGUI_ENABLED=1");
			PublicDefinitions.Add("NETIMGUI_LISTENPORT_GAME=" + kGameListenPort);
			PublicDefinitions.Add("NETIMGUI_LISTENPORT_EDITOR=" + kEditorListenPort);
			PublicDefinitions.Add("NETIMGUI_LISTENPORT_DEDICATED_SERVER=" + kServerListenPort);
			
			PublicDefinitions.Add("NETIMGUI_WINSOCKET_ENABLED=0");      // Using Unreal sockets, no need for built-in sockets
			PublicDefinitions.Add("NETIMGUI_POSIX_SOCKETS_ENABLED=0");  // Using Unreal sockets, no need for built-in sockets
		}
		else
		{
			PublicDefinitions.Add("NETIMGUI_ENABLED=0");
		}
	}
}
