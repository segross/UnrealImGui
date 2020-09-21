#pragma once

#include "NetImgui_Shared.h"

//=============================================================================
// Forward Declares
//=============================================================================
namespace NetImgui { namespace Internal { struct CmdTexture; struct CmdDrawFrame; struct CmdInput; } }
namespace NetImgui { namespace Internal { namespace Network { struct SocketInfo; } } }

namespace NetImgui { namespace Internal { namespace Client
{

//=============================================================================
// Keep a list of textures used by Imgui, needed by server
//=============================================================================
struct ClientTexture
{
	inline void	Set( CmdTexture* pCmdTexture );
	inline bool	IsValid()const;	
	CmdTexture* mpCmdTexture= nullptr;
	bool		mbSent		= false;
	uint8_t		mPadding[7]	= {0};
};

//=============================================================================
// Keep all Client infos needed for communication with server
//=============================================================================
struct ClientInfo
{
	using VecTexture	= ImVector<ClientTexture>;
	using BufferKeys	= Ringbuffer<uint16_t, 1024>;
	using Time			= std::chrono::time_point<std::chrono::high_resolution_clock>;

										ClientInfo();
	std::atomic<Network::SocketInfo*>	mpSocketPending;						// Hold socket info until communication is established
	std::atomic<Network::SocketInfo*>	mpSocketComs;							// Socket used for communications with server
	std::atomic<Network::SocketInfo*>	mpSocketListen;							// Socket used to wait for communication request from server
	char								mName[16]					={0};
	VecTexture							mTextures;
	CmdTexture*							mTexturesPending[16];
	ExchangePtr<CmdDrawFrame>			mPendingFrameOut;
	ExchangePtr<CmdInput>				mPendingInputIn;
	BufferKeys							mPendingKeyIn;
	ImGuiContext*						mpContextClone				= nullptr;	// Default ImGui drawing context copy, used to do our internal remote drawing
	ImGuiContext*						mpContextEmpty				= nullptr;	// Placeholder ImGui drawing context, when we are not waiting for a new drawing frame but still want a valid context in place
	ImGuiContext*						mpContextRestore			= nullptr;	// Context to restore to Imgui once drawing is done
	ImGuiContext*						mpContextDrawing			= nullptr;	// Current context used for drawing (between a BeginFrame/EndFrame)
	Time								mTimeTracking;
	std::atomic_int32_t					mTexturesPendingCount;	
	float								mMouseWheelVertPrev			= 0.f;
	float								mMouseWheelHorizPrev		= 0.f;
	int									mRestoreKeyMap[ImGuiKey_COUNT];
	ImGuiConfigFlags					mRestoreConfigFlags			= 0;
	const char*							mRestoreBackendPlatformName	= nullptr;
	const char*							mRestoreBackendRendererName	= nullptr;	
	ImGuiBackendFlags					mRestoreBackendFlags		= 0;
	bool								mbDisconnectRequest			= false;	// Waiting to Disconnect
	bool								mbHasTextureUpdate			= false;
	bool								mbIsRemoteDrawing			= false;	// True if the rendering it meant for the remote netImgui server
	bool								mbRestorePending			= false;	// Original context has had some settings overridden, original values stored in mRestoreXXX	
	//char								PADDING[5];
	void								TextureProcessPending();
	void								TextureProcessRemoval();
	inline bool							IsConnected()const;
	inline bool							IsConnectPending()const;
	inline bool							IsActive()const;
	inline void							KillSocketComs();
	inline void							KillSocketListen();
	inline void							KillSocketAll();

// Prevent warnings about implicitly created copy
protected:
	ClientInfo(const ClientInfo&)=delete;
	ClientInfo(const ClientInfo&&)=delete;
	void operator=(const ClientInfo&)=delete;
};

//=============================================================================
// Main communication thread, that should be started in its own thread
//=============================================================================
void CommunicationsClient(void* pClientVoid);
void CommunicationsHost(void* pClientVoid);

}}} //namespace NetImgui::Internal::Client

#include "NetImgui_Client.inl"
