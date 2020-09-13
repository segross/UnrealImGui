#pragma once

//=================================================================================================
// Set the path to 'imgui.h' used by your codebase here. 
// Also suppress a few warnings imgui.h generates in 'warning All' (-Wall)
//=================================================================================================
#include "Private/NetImgui_WarningDisableImgui.h" 
#include <imgui.h>
#include "Private/NetImgui_WarningReenable.h"

//=================================================================================================
// Port used by connect the Server and Client together
//=================================================================================================
namespace NetImgui
{
constexpr uint32_t kDefaultServerPort = 8888;	//!< Default port Server waits for a connection
constexpr uint32_t kDefaultClientPort = 8889;	//!< Default port Client waits for a connection
}

//=================================================================================================
// Enable code compilation for this library
// Note: Useful to disable 'netImgui' on unsupported builds while keeping functions declared
//=================================================================================================
#ifndef NETIMGUI_ENABLED
	#define NETIMGUI_ENABLED	1
#endif

//=================================================================================================
// Enable default Win32/Posix networking code
// Note:	By default, netImgui uses Winsock on Windows and Posix sockets on other platforms
//
//			The use your own code, turn off both NETIMGUI_WINSOCKET_ENABLED, 
//			NETIMGUI_POSIX_SOCKETS_ENABLED and provide your own implementation of the functions 
//			declared in 'NetImgui_Network.h'.
//=================================================================================================
#if !defined(NETIMGUI_WINSOCKET_ENABLED) && !defined(__UNREAL__)
	#ifdef _WIN32
		#define NETIMGUI_WINSOCKET_ENABLED	1 // Project needs 'ws2_32.lib' added to input libraries
	#else
		#define NETIMGUI_WINSOCKET_ENABLED	0
	#endif
#endif

#if !defined(NETIMGUI_POSIX_SOCKETS_ENABLED) && !defined(__UNREAL__)
	#define NETIMGUI_POSIX_SOCKETS_ENABLED	!(NETIMGUI_WINSOCKET_ENABLED)
#endif
