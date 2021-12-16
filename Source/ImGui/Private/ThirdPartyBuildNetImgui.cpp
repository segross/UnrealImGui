// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ThirdPartyBuildNetImgui.h"

#if NETIMGUI_ENABLED
#include "CoreMinimal.h"
#include "ImGuiContextManager.h"
#include "ImGuiDelegatesContainer.h"

enum : int { kDualUI_None, kDualUI_Mirror, kDualDisplay_On };

static int					sRemoteContextIndex		= Utilities::INVALID_CONTEXT_INDEX;	// Which proxy context is currently associated with netImgui
static int					sDualUIType				= kDualUI_None;						// How to handle the local ImGui display when connected remotely
static FImGuiContextProxy*	spActiveContextProxy	= nullptr;
static ImGuiContext*		spNetImguiContext		= nullptr;

//=================================================================================================
// Pick the most appropriate port to wait for connection
//=================================================================================================
uint32_t GetListeningPort()
{
	if (IsRunningDedicatedServer())
	{
		return NETIMGUI_LISTENPORT_DEDICATED_SERVER;
	}
	else if (FApp::IsGame())
	{
		return NETIMGUI_LISTENPORT_GAME;
	}
	else
	{
		return NETIMGUI_LISTENPORT_EDITOR;
	}
}

//=================================================================================================
// If not already done, start listening for netImgui server to connect to us
// Note: Ask Editor to wait on a different port from Game, so both can connected at the same time
// Note: We Clone the context, so original ImGui context is left unmodified
//=================================================================================================
void NetImguiPreUpdate_Connection()
{
	// Only listen for connection from netImGui server, 
	// Could also support reaching server directly if we had a provided IP
	if (ImGui::GetCurrentContext() && !NetImgui::IsConnected() && !NetImgui::IsConnectionPending())
	{
		// Using a separate Imgui Context for NetImgui, so ContextProxy can be easily swapped to any active (PIE, Editor, Game, ...)
		if( !spNetImguiContext )
		{
			spNetImguiContext = ImGui::CreateContext(ImGui::GetIO().Fonts);
		}
		ImGui::SetCurrentContext(spNetImguiContext);

		FString sessionName = FString::Format(TEXT("{0}-{1}"), { FApp::GetProjectName(), FPlatformProcess::ComputerName() });

		// Setup connection to wait for netImgui server to reach us
		NetImgui::ConnectFromApp(TCHAR_TO_ANSI(sessionName.GetCharArray().GetData()), GetListeningPort());

		// Setup connection to try reaching netImgui server directly
		//NetImgui::ConnectToApp(TCHAR_TO_ANSI(sessionName.GetCharArray().GetData()), "localhost", NetImgui::kDefaultServerPort, true);
	}
}

//=================================================================================================
// Find which proxy context as been selected for remote draw, to take it over
//=================================================================================================
void NetImguiPreUpdate_FindActiveContext(TMap<int32, FContextData>& Contexts)
{
	spActiveContextProxy = nullptr;
	if( NetImgui::IsConnected() )
	{
		for (auto& Pair : Contexts)
		{
			// Pick 1st available context when not initalized
			if( sRemoteContextIndex == Utilities::INVALID_CONTEXT_INDEX )
				sRemoteContextIndex = static_cast<int>(Pair.Key);

			// Check if this is the context we want
			auto& ContextData = Pair.Value;
			bool bIsRemote = NetImgui::IsConnected() && ContextData.CanTick() && static_cast<int>(Pair.Key) == sRemoteContextIndex;
			if( bIsRemote )
			{
				spActiveContextProxy = ContextData.ContextProxy.Get();
			}
		}
	}
}

//=================================================================================================
// Finish and start a new netImgui frame
//=================================================================================================
void NetImguiPreUpdate_NextFrame()
{
	if (NetImgui::IsConnected())
	{
		ImGui::SetCurrentContext(spNetImguiContext);
		NetImgui::EndFrame();

		if (spActiveContextProxy)
		{
			// User requested to have NetImgui content also displayed locally. We need to 
			// do this here, right after the EndFrame, otherwise the ImguiData data will be null
			if (sDualUIType == kDualUI_Mirror)
			{
				ImDrawData* pDrawData = ImGui::GetDrawData();
				// Because UpdateDrawData take ownership of the Imgui DrawData with a memory swap, 
				// we make sure we only update the display when we have new draw data in, instead of 
				// relying on NetImgui DrawData that got stolen by the ProxyContext
				if (pDrawData && pDrawData->CmdListsCount > 0)
				{
					spActiveContextProxy->UpdateDrawData(pDrawData);
					pDrawData->CmdListsCount = 0;
				}
			}
			// Clear the local display
			else if (sDualUIType == kDualUI_None)
			{
				spActiveContextProxy->UpdateDrawData(nullptr);
			}
		}
	
		// It is possible to avoid drawing ImGui when connected and server doesn't expect a new frame,
		// but requires to skip calling drawing delegates and user not to draw in UObject::Tick. 
		// Last point difficult to control, so might be safer to not support 'frameskip'
		NetImgui::NewFrame();
	}
}

//=================================================================================================
// Add a main menu bar entry, with a list of Context to choose from.
//=================================================================================================
void NetImguiPreUpdate_DrawNetImguiContent(TMap<int32, FContextData>& Contexts)
{	
	if ( ImGui::BeginMainMenuBar() )
	{
		if (ImGui::BeginMenu("NetImgui"))
		{
			for (auto& Pair : Contexts)
			{
				auto& context = Pair.Value;
				FString name = FString::Format(TEXT("{0} {1}"), { context.ContextProxy->GetName(), context.CanTick() ? TEXT("") : TEXT("(Inactive)") });
				ImGui::RadioButton(TCHAR_TO_ANSI(name.GetCharArray().GetData()), &sRemoteContextIndex, static_cast<int>(Pair.Key));
			}

			// Add other netImgui options here, like continue display locally
			ImGui::Separator();
			ImGui::RadioButton("DualDisplay: None", &sDualUIType, kDualUI_None);
			ImGui::RadioButton("DualDisplay: Mirror", &sDualUIType, kDualUI_Mirror);
			ImGui::RadioButton("DualDisplay: On", &sDualUIType, kDualDisplay_On);
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("Display different Dear ImGui content in Game and NetImgui server simultaneously.\nYour Imgui code must support multi-context callback to work with this.");
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}	
}

#endif // NETIMGUI_ENABLED

//=================================================================================================
// (Public) Initialize the library
//=================================================================================================
void NetImGuiStartup()
{
#if NETIMGUI_ENABLED
	NetImgui::Startup();
#endif
}

//=================================================================================================
// (Public) Delete some resources before shutdown
//=================================================================================================
void NetImGuiShutdown()
{
#if NETIMGUI_ENABLED
	NetImgui::Shutdown(true);
	if( spNetImguiContext )
	{
		ImGui::DestroyContext(spNetImguiContext);
		spNetImguiContext = nullptr;
	}
#endif
}
//=================================================================================================
// (Public) Main update of netImgui. Establish connection and call delegates associated with it
//			to receive their ImGui draw commands.
//=================================================================================================
void NetImguiUpdate(TMap<int32, FContextData>& Contexts)
{
#if NETIMGUI_ENABLED
	NetImguiPreUpdate_Connection();
	NetImguiPreUpdate_FindActiveContext(Contexts);
	NetImguiPreUpdate_NextFrame();
	if (NetImgui::IsDrawingRemote())
	{
		NetImguiPreUpdate_DrawNetImguiContent(Contexts);
	}
#else
	(void)Contexts;
#endif
}

//=================================================================================================
// (Public) If this Proxy Context should be doing ImGui drawing
//=================================================================================================
bool NetImGuiCanDrawProxy(const FImGuiContextProxy* pProxyContext)
{
#if NETIMGUI_ENABLED
	return pProxyContext != spActiveContextProxy || sDualUIType == kDualDisplay_On;
#else
	return true;
#endif
}

//=================================================================================================
// (Public) Set the remote ImGui context as current if provided Proxy context 
//			is the one we are assigned to override
//=================================================================================================
bool NetImGuiSetupDrawRemote(const FImGuiContextProxy* pProxyContext)
{
#if NETIMGUI_ENABLED
	if( pProxyContext == spActiveContextProxy )
	{
		ImGui::SetCurrentContext(spNetImguiContext);
		return NetImgui::IsDrawingRemote();
	}
#endif
	return false;
}

//=================================================================================================
// For convenience and easy access to the netImgui source code, we build it as part of this module.
//=================================================================================================
#if NETIMGUI_ENABLED

#include <Private/NetImgui_Api.cpp>
#include <Private/NetImgui_Client.cpp>
#include <Private/NetImgui_CmdPackets_DrawFrame.cpp>
#include <Private/NetImgui_NetworkUE4.cpp>

#endif // NETIMGUI_ENABLED
