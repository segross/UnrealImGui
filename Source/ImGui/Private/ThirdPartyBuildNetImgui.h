// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#if defined(NETIMGUI_ENABLED) && NETIMGUI_ENABLED
	#include <NetImgui_Api.h>		
	void NetImguiUpdate(TMap<int32, struct FContextData>& Contexts);
	
#elif !defined(NETIMGUI_ENABLED)
	#define NETIMGUI_ENABLED 0

#endif
