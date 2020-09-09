// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ThirdPartyBuildNetImgui.h"

#if NETIMGUI_ENABLED

#include "ImGuiContextManager.h"

static int sRemoteContextIndex = 0; // Which context is currently associated with netImgui
static bool sMirrorContent = false;

bool NetImguiDisplayMirror()
{
	return sMirrorContent;
}

void NetImguiUpdate(TMap<int32, FContextData>& Contexts)
{
	//------------------------------------------------------------------------------------------------
	// If not already done, start listening for netImgui server to connect to us
	// Note: Ask Editor to wait on a different port from Game, so both can connected at the same time
	// Note: We Clone the context, so original ImGui context is left unmodified
	//------------------------------------------------------------------------------------------------
	if( ImGui::GetCurrentContext() && !NetImgui::IsConnected() && !NetImgui::IsConnectionPending())
	{
		FString sessionName = FString::Format(TEXT("{0}-{1}"), { FApp::GetProjectName(), FPlatformProcess::ComputerName() });
		NetImgui::ConnectFromApp(TCHAR_TO_ANSI(sessionName.GetCharArray().GetData()), FApp::IsGame() ? NETIMGUI_LISTENPORT_GAME : NETIMGUI_LISTENPORT_EDITOR, true);
	}

	//------------------------------------------------------------------------------------------------
	// Update all context 'remote' status, and find which one is associated with netImgui
	//------------------------------------------------------------------------------------------------
	FImGuiContextProxy* pRemoteContextProxy(nullptr);
	for (auto& Pair : Contexts)
	{
		auto& ContextData = Pair.Value;
		bool bIsRemote = NetImgui::IsConnected() && ContextData.CanTick() && static_cast<int>(Pair.Key) == sRemoteContextIndex;
		pRemoteContextProxy = bIsRemote ? ContextData.ContextProxy.Get() : pRemoteContextProxy;
		ContextData.ContextProxy->SetIsRemoteDraw(bIsRemote);
	}

	//------------------------------------------------------------------------------------------------
	// Finish and start a new netImgui frame
	//------------------------------------------------------------------------------------------------
	if (NetImgui::IsConnected())
	{
		if (NetImgui::GetDrawingContext() != nullptr)
			NetImgui::EndFrame();
			
		if (pRemoteContextProxy)
		{
			// User requested to have netImgui content also displayed locally
			// We need to do this here, right after the EndFrame, otherwise the ImguiData 
			// data will be null
			if( sMirrorContent )
			{
				ImDrawData* pDrawData = NetImgui::GetDrawData();
				// Because UpdateDrawData take ownership of the Imgui DrawData with a memory swap, 
				// we make sure we only update the display when we have new draw data in, instead of 
				// relying on NetImgui DrawData that got stolen by the ProxyContext
				if(pDrawData->CmdListsCount > 0)
				{
					pRemoteContextProxy->UpdateDrawData(pDrawData);
					pDrawData->CmdListsCount = 0; 
				}
			}
			// Clear the local display
			else
			{
				pRemoteContextProxy->UpdateDrawData(nullptr);
			}

			// NetImgui::NewFrame update delta time of original context (to match netImgui)
			// make sure it updates the right one
			pRemoteContextProxy->SetAsCurrent();
			
		}
		// It is possible to avoid drawing ImGui when connected and server doesn't expect a new frame,
		// but requires to skip calling drawing delegates and user not to draw in UObject::Tick. 
		// Last point difficult to control, so might be safer to not support 'frameskip'
		NetImgui::NewFrame();

		// Add a main menu bar with a list of Context to choose from.
		// Could add more options (like continue drawing locally)
		bool bShouldDisplayMenu = true; //(Contexts.Num() > 1 || pRemoteContextProxy == nullptr);
		if (bShouldDisplayMenu && ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("netImgui"))
			{
				for (auto& Pair : Contexts)
				{
					auto& context = Pair.Value;
					FString name = FString::Format(TEXT("({0}) {1} {2}"), { static_cast<int>(Pair.Key), context.ContextProxy->GetName(), context.CanTick() ? TEXT("") : TEXT("(Inactive)") });
					ImGui::RadioButton(TCHAR_TO_ANSI(name.GetCharArray().GetData()), &sRemoteContextIndex, static_cast<int>(Pair.Key));
				}

				// Add other netImgui options here, like continue display locally
				ImGui::Separator();
				ImGui::Checkbox("Mirror ImGui in game", &sMirrorContent);

				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	}
}

// For convenience and easy access to the netImgui source code, we build it as part of this module.
#include <Private/NetImgui_Api.cpp>
#include <Private/NetImgui_Client.cpp>
#include <Private/NetImgui_CmdPackets_DrawFrame.cpp>
#include <Private/NetImgui_NetworkUE4.cpp>

#endif // NETIMGUI_ENABLED
