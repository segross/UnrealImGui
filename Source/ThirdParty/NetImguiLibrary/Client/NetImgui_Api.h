#pragma once

//=================================================================================================
//! @Name		: netImgui
//=================================================================================================
//! @author		: Sammy Fatnassi
//! @date		: 2020/09/06
//!	@version	: v1.3.0
//! @Details	: For integration info : https://github.com/sammyfreg/netImgui/wiki
//=================================================================================================
#define NETIMGUI_VERSION		"1.3.0"
#define NETIMGUI_VERSION_NUM	10300

struct ImGuiContext;
struct ImDrawData;

#include <stdint.h>

#include "Private/NetImgui_WarningDisable.h"
#include "NetImgui_Config.h"

//=================================================================================================
// NetImgui needs to detect Dear ImGui to be active
//=================================================================================================
#ifndef NETIMGUI_ENABLED
	#define NETIMGUI_ENABLED 0
#elif !defined(IMGUI_VERSION)
	#undef	NETIMGUI_ENABLED
	#define	NETIMGUI_ENABLED 0
#endif

//=================================================================================================
// List of texture format supported
//=================================================================================================
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
// Try to establish a connection to netImguiApp server. 
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
// bCloneContext	: When false, BeginFrame will rely on current dear ImGui DrawContext, so 
//					  transition between local and remote is seamless. 
//					  When true, creates a duplicate of the current context, so netImgui can do 
//					  its own drawing without affecting the original content. Useful when you want
//					  to display some content locally and remotely simultaneously.
// threadFunction	: User provided function to launch a new thread running the function
//					  received as a parameter. Use 'DefaultStartCommunicationThread'
//					  by default, which relies on 'std::thread'.
//=================================================================================================
bool				ConnectToApp(const char* clientName, const char* serverHost, uint32_t serverPort=kDefaultServerPort, bool bCloneContext=false);
bool				ConnectToApp(ThreadFunctPtr threadFunction, const char* clientName, const char* ServerHost, uint32_t serverPort = kDefaultServerPort, bool bCloneContext=false);
bool				ConnectFromApp(const char* clientName, uint32_t clientPort=kDefaultClientPort, bool bCloneContext=false);
bool				ConnectFromApp(ThreadFunctPtr threadFunction, const char* clientName, uint32_t serverPort=kDefaultClientPort, bool bCloneContext=false);

//=================================================================================================
// Request a disconnect from the netImguiApp server
//=================================================================================================
void				Disconnect(void);

//=================================================================================================
// True if connected to netImguiApp server
//=================================================================================================
bool				IsConnected(void);

//=================================================================================================
// True if connection request is waiting to be completed. Waiting for Server to connect to us 
// after having called 'ConnectFromApp()' for example
//=================================================================================================
bool				IsConnectionPending(void);

//=================================================================================================
// True when Dear ImGui is currently expecting draw commands 
// This means that we are between NewFrame() and EndFrame() of drawing for remote application
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
void				SendDataTexture(uint64_t textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format);

//=================================================================================================
// Start a new Imgui Frame and wait for Draws commands, using a ImGui created internally
// for remote drawing. Returns true if we are awaiting a new ImGui frame. 
//
// All ImGui drawing can be skip when false.
//
// Note: This code can be used instead, to know if you should be drawing or not :
//			'if( !NetImgui::IsDrawing() )'
//
// Note: If your code cannot handle skipping a ImGui frame, set 'bSupportFrameSkip' to false,
//		 and an empty ImGui context will be assigned when no drawing is needed
//=================================================================================================
bool				NewFrame(bool bSupportFrameSkip=false);

//=================================================================================================
// Process all receives draws, send them to remote connection and restore the ImGui Context
//=================================================================================================
void				EndFrame(void);

//=================================================================================================
// 
//=================================================================================================
ImGuiContext*		GetDrawingContext();

//=================================================================================================
// Regular ImGui draw data, from the last valid draw.
// Note: Be careful with the returned value, the pointer remain valid only as long as
//		 a new dear ImGui frame hasn't been started for the netImgui remote app
//=================================================================================================
ImDrawData*			GetDrawData(void);

uint8_t				GetTexture_BitsPerPixel	(eTexFormat eFormat);
uint32_t			GetTexture_BytePerLine	(eTexFormat eFormat, uint32_t pixelWidth);
uint32_t			GetTexture_BytePerImage	(eTexFormat eFormat, uint32_t pixelWidth, uint32_t pixelHeight);
} 

#include "Private/NetImgui_WarningReenable.h"
