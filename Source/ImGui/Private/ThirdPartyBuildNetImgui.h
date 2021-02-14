// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiContextManager.h"

#if defined(NETIMGUI_ENABLED) && NETIMGUI_ENABLED
	#include <NetImgui_Api.h>

#elif !defined(NETIMGUI_ENABLED)
	#define NETIMGUI_ENABLED 0

#endif

void	NetImGuiStartup();

void	NetImGuiShutdown();

// Called at the start of updating the Proxy context
void	NetImguiUpdate(TMap<int32, FContextData>& Contexts);

// Let caller know if this proxy can be draw normally
bool	NetImGuiCanDrawProxy(const FImGuiContextProxy* pProxyContext);

// Setup remote drawing context and let caller know if netImgui can draw remotely, using this proxy delegates
bool	NetImGuiSetupDrawRemote(const FImGuiContextProxy* pProxyContext);