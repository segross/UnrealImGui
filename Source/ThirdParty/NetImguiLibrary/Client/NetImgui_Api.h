#pragma once

//=================================================================================================
//! @Name		: NetImgui
//=================================================================================================
//! @author		: Sammy Fatnassi
//! @date		: 2021/07/17
//!	@version	: v1.5.2
//! @Details	: For integration info : https://github.com/sammyfreg/netImgui/wiki
//=================================================================================================
#define NETIMGUI_VERSION		"1.5.2 WIP"
#define NETIMGUI_VERSION_NUM	10502

#include <stdint.h>
#include "Private/NetImgui_WarningDisable.h"

#ifdef NETIMGUI_IMPLEMENTATION
	#define NETIMGUI_INTERNAL_INCLUDE
#endif
#include "NetImgui_Config.h"
#ifdef NETIMGUI_IMPLEMENTATION
	#undef NETIMGUI_INTERNAL_INCLUDE
#endif

//=================================================================================================
// If 'NETIMGUI_ENABLED' hasn't been defined yet (in project settings or NetImgui_Config.h') 
// we define this library as 'Disabled'
//=================================================================================================
#if !defined(NETIMGUI_ENABLED)
	#define NETIMGUI_ENABLED 0
#endif

// NetImgui needs to detect Dear ImGui to be active, otherwise we disable it
#if !defined(IMGUI_VERSION)
	#undef	NETIMGUI_ENABLED
	#define	NETIMGUI_ENABLED 0
#endif

//=================================================================================================
// List of texture format supported
//=================================================================================================
#if NETIMGUI_ENABLED
namespace NetImgui 
{ 
enum class eTexFormat : uint8_t { 
	kTexFmtA8, 
	kTexFmtRGBA8, 
	kTexFmt_Count,
	kTexFmt_Invalid=kTexFmt_Count };

typedef void		ThreadFunctPtr(void threadedFunction(void* pClientInfo), void* pClientInfo) ;

//=================================================================================================
// Initialize the Network Library
//=================================================================================================
bool				Startup(void);

//=================================================================================================
// Free Resources
//-------------------------------------------------------------------------------------------------
// bWait			: Wait until all communication threads have terminated before returning
//=================================================================================================
void				Shutdown(bool bWait);

//=================================================================================================
// Try to establish a connection to NetImgui server application. 
//
// Can establish connection with netImgui Server application by either reaching it directly
// using 'ConnectToApp' or waiting for Server to reach us after Client called 'ConnectFromApp'.
//
// Note:	Start a new communication thread using std::Thread by default, but can receive custom 
//			thread start function instead (Look at ClientExample 'CustomCommunicationThread').
//-------------------------------------------------------------------------------------------------
// clientName		: Named that will be displayed on the Server, for this Client
// serverHost		: Address of the netImgui Server application (Ex1: 127.0.0.2, Ex2: localhost)
// serverPort		: PortID of the netImgui Server application to connect to
// clientPort		: PortID this Client should wait for connection from Server application
// threadFunction	: User provided function to launch new networking thread.
//					  Use 'DefaultStartCommunicationThread' by default, relying on 'std::thread'.
//=================================================================================================
bool				ConnectToApp(const char* clientName, const char* serverHost, uint32_t serverPort=kDefaultServerPort, ThreadFunctPtr threadFunction=nullptr);
bool				ConnectFromApp(const char* clientName, uint32_t clientPort=kDefaultClientPort, ThreadFunctPtr threadFunction = nullptr);

//=================================================================================================
// Request a disconnect from the netImguiApp server
//=================================================================================================
void				Disconnect(void);

//=================================================================================================
// True if connected to netImguiApp server
//=================================================================================================
bool				IsConnected(void);

//=================================================================================================
// True if connection request is waiting to be completed. For example, while waiting for  
// Server to reach ud after having called 'ConnectFromApp()'
//=================================================================================================
bool				IsConnectionPending(void);

//=================================================================================================
// True when Dear ImGui is currently expecting draw commands 
// This means that we are between NewFrame() and EndFrame() 
//=================================================================================================
bool				IsDrawing(void);

//=================================================================================================
// True when Dear ImGui is currently expecting draw commands *sent to remote netImgui app*.
// This means that we are between NewFrame() and EndFrame() of drawing for remote application
//=================================================================================================
bool				IsDrawingRemote(void);

//=================================================================================================
// Send an updated texture used by imgui, to netImguiApp server
// Note: To remove a texture, set pData to nullptr
//=================================================================================================
void				SendDataTexture(ImTextureID textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format);

//=================================================================================================
// Start a new Imgui Frame and wait for Draws commands, using a ImGui created internally
// for remote drawing. Returns true if we are awaiting a new ImGui frame. 
//
// All ImGui drawing can be skip when false.
//
// Note: This code can be used instead, to know if you should be drawing or not :
//			'if( !NetImgui::IsDrawing() )'
//
// Note: If your code cannot handle skipping a ImGui frame, leave 'bSupportFrameSkip==false',
//		 and an empty ImGui context will be assigned to receive discarded drawing commands
//
// Note: With Dear ImGui 1.81+, you can keep using the ImGui::BeginFrame()/Imgui::Render()
//		 without having to use these 2 functions.
//=================================================================================================
bool				NewFrame(bool bSupportFrameSkip=false);

//=================================================================================================
// Process all receives draws, send them to remote connection and restore the ImGui Context
//=================================================================================================
void				EndFrame(void);

//=================================================================================================
// Return the context associated to this remote connection. Null when not connected.
//=================================================================================================
ImGuiContext*		GetContext();

//=================================================================================================
// Set the remote client backoground color and texture
// Note: If no TextureID is specified, will use the default server texture
//=================================================================================================
void				SetBackground(const ImVec4& bgColor);
void				SetBackground(const ImVec4& bgColor, const ImVec4& textureTint );
void				SetBackground(const ImVec4& bgColor, const ImVec4& textureTint, ImTextureID bgTextureID);

//=================================================================================================
// Helper function to quickly create a context duplicate (sames settings/font/styles)
//=================================================================================================
uint8_t				GetTexture_BitsPerPixel	(eTexFormat eFormat);
uint32_t			GetTexture_BytePerLine	(eTexFormat eFormat, uint32_t pixelWidth);
uint32_t			GetTexture_BytePerImage	(eTexFormat eFormat, uint32_t pixelWidth, uint32_t pixelHeight);
} 

//=================================================================================================
// Optional single include compiling option
// Note: User that do not wish adding the few NetImgui cpp files to their project,
//		 can instead define 'NETIMGUI_IMPLEMENTATION' *once* before including 'NetImgui_Api.h'
//		 and this will load the required cpp files alongside
//=================================================================================================
#if defined(NETIMGUI_IMPLEMENTATION)

#include "Private/NetImgui_Api.cpp"
#include "Private/NetImgui_Client.cpp"
#include "Private/NetImgui_CmdPackets_DrawFrame.cpp"
#include "Private/NetImgui_NetworkPosix.cpp"
#include "Private/NetImgui_NetworkUE4.cpp"
#include "Private/NetImgui_NetworkWin32.cpp"

#endif
#endif // NETIMGUI_ENABLED

#include "Private/NetImgui_WarningReenable.h"
