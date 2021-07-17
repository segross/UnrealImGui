#include "NetImgui_Shared.h"
#include "NetImgui_WarningDisable.h"

#if NETIMGUI_ENABLED
#include <algorithm>
#include <thread>
#include "NetImgui_Client.h"
#include "NetImgui_Network.h"
#include "NetImgui_CmdPackets.h"

using namespace NetImgui::Internal;

namespace NetImgui { 

static Client::ClientInfo* gpClientInfo = nullptr;

bool ProcessInputData(Client::ClientInfo& client);

//=================================================================================================
void DefaultStartCommunicationThread(void ComFunctPtr(void*), void* pClient)
//=================================================================================================
{
// Visual Studio 2017 generate this warning on std::thread, avoid the warning preventing build
#if defined(_MSC_VER) && (_MSC_VER < 1920)
	#pragma warning	(push)		
	#pragma warning (disable: 4625)		// 'std::_LaunchPad<_Target>' : copy constructor was implicitly defined as deleted
	#pragma warning (disable: 4626)		// 'std::_LaunchPad<_Target>' : assignment operator was implicitly defined as deleted
#endif

	std::thread(ComFunctPtr, pClient).detach();

#if defined(_MSC_VER) && (_MSC_VER < 1920)
	#pragma warning	(pop)
#endif	
}


//=================================================================================================
bool ConnectToApp(const char* clientName, const char* ServerHost, uint32_t serverPort, ThreadFunctPtr threadFunction)
//=================================================================================================
{
	if (!gpClientInfo) return false;

	Client::ClientInfo& client	= *gpClientInfo;	
	Disconnect();
	
	while (client.IsActive())
		std::this_thread::yield();

	StringCopy(client.mName, (clientName == nullptr || clientName[0] == 0 ? "Unnamed" : clientName));
	client.mpSocketPending	= Network::Connect(ServerHost, serverPort);	
	if (client.mpSocketPending.load() != nullptr)
	{				
		client.ContextInitialize();
		threadFunction		= threadFunction == nullptr ? DefaultStartCommunicationThread : threadFunction;
		threadFunction(Client::CommunicationsClient, &client);
	}
	
	return client.IsActive();
}

//=================================================================================================
bool ConnectFromApp(const char* clientName, uint32_t serverPort, ThreadFunctPtr threadFunction)
//=================================================================================================
{
	if (!gpClientInfo) return false;

	Client::ClientInfo& client = *gpClientInfo;
	Disconnect();

	while (client.IsActive())
		std::this_thread::yield();

	StringCopy(client.mName, (clientName == nullptr || clientName[0] == 0 ? "Unnamed" : clientName));
	client.mpSocketPending = Network::ListenStart(serverPort);
	if (client.mpSocketPending.load() != nullptr)
	{				
		client.ContextInitialize();
		threadFunction		= threadFunction == nullptr ? DefaultStartCommunicationThread : threadFunction;
		threadFunction(Client::CommunicationsHost, &client);
	}

	return client.IsActive();
}

//=================================================================================================
void Disconnect(void)
//=================================================================================================
{
	if (!gpClientInfo) return;
	
	Client::ClientInfo& client	= *gpClientInfo;
	client.mbDisconnectRequest	= client.IsActive();
}

//=================================================================================================
bool IsConnected(void)
//=================================================================================================
{
	if (!gpClientInfo) return false;
	
	Client::ClientInfo& client = *gpClientInfo;

	// If disconnected in middle of a remote frame drawing,  
	// want to behave like it is still connected to finish frame properly
	return client.IsConnected() || IsDrawingRemote(); 
}

//=================================================================================================
bool IsConnectionPending(void)
//=================================================================================================
{
	if (!gpClientInfo) return false;
	
	Client::ClientInfo& client = *gpClientInfo;
	return client.IsConnectPending();
}

//=================================================================================================
bool IsDrawing(void)
//=================================================================================================
{
	if (!gpClientInfo) return false;

	Client::ClientInfo& client = *gpClientInfo;
	return client.mbIsDrawing;
}

//=================================================================================================
bool IsDrawingRemote(void)
//=================================================================================================
{
	if (!gpClientInfo) return false;

	Client::ClientInfo& client = *gpClientInfo;
	return IsDrawing() && client.mbIsRemoteDrawing;
}

//=================================================================================================
bool NewFrame(bool bSupportFrameSkip)
//=================================================================================================
{	
	if (!gpClientInfo) return false;

	Client::ClientInfo& client = *gpClientInfo;	
	ScopedBool scopedInside(client.mbInsideNewEnd, true);
	assert(!client.mbIsDrawing);

	// ImGui Newframe handled by remote connection settings	
	if( NetImgui::IsConnected() )
	{		
		ImGui::SetCurrentContext(client.mpContext);

		// Save current context settings and override settings to fit our netImgui usage 
		if (!client.IsContextOverriden() )
		{			
			client.ContextOverride();
		}
		
		// Update input and see if remote netImgui expect a new frame
		client.mSavedDisplaySize	= ImGui::GetIO().DisplaySize;
		client.mbValidDrawFrame		= ProcessInputData(client);
		
		// We are about to start drawing for remote context, check for font data update
		const ImFontAtlas* pFonts = ImGui::GetIO().Fonts;
		if( pFonts->TexPixelsAlpha8 && 
			(pFonts->TexPixelsAlpha8 != client.mpFontTextureData || client.mFontTextureID != pFonts->TexID ))
		{
			uint8_t* pPixelData(nullptr); int width(0), height(0);
			ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&pPixelData, &width, &height);
			SendDataTexture(ImGui::GetIO().Fonts->TexID, pPixelData, static_cast<uint16_t>(width), static_cast<uint16_t>(height), eTexFormat::kTexFmtA8);
		}

		// No font texture has been sent to the netImgui server, you can either 
		// 1. Leave font data available in ImGui (not call ImGui::ClearTexData) for netImgui to auto send it
		// 2. Manually call 'NetImgui::SendDataTexture' with font texture data
		assert(client.mbFontUploaded);
			
		// Update current active content with our time
		const auto TimeNow						= std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> elapsedSec	= TimeNow - client.mTimeTracking;
		ImGui::GetIO().DeltaTime				= std::max<float>(1.f / 1000.f, elapsedSec.count());
		client.mTimeTracking					= TimeNow;
		
		// NetImgui isn't waiting for a new frame, try to skip drawing when caller supports it
		if( !client.mbValidDrawFrame && bSupportFrameSkip )
		{
			return false;
		}	
	}
	// Regular Imgui NewFrame
	else
	{
		// Restore context setting override, after a disconnect 		
		client.ContextRestore();
		
		// Remove hooks callback only when completly disconnected
		if (!client.IsConnectPending())
		{
			client.ContextRemoveHooks();
		}
	}

	// A new frame is expected, update the current time of the drawing context, and let Imgui know to prepare a new drawing frame	
	client.mbIsRemoteDrawing	= NetImgui::IsConnected();
	client.mbIsDrawing			= true;
	
	// This function can be called from a 'NewFrame' ImGui hook, we should not start a new frame again
	if (!client.mbInsideHook)
	{
		ImGui::NewFrame();
	}

	return true;
}

//=================================================================================================
void EndFrame(void)
//=================================================================================================
{
	if (!gpClientInfo) return;

	Client::ClientInfo& client = *gpClientInfo;	
	ScopedBool scopedInside(client.mbInsideNewEnd, true);

	if ( client.mbIsDrawing  )
	{
		// Must be fetched before 'Render'
		ImGuiMouseCursor Cursor	= ImGui::GetMouseCursor();	
		
		// This function can be called from a 'NewFrame' ImGui hook, in which case no need to call this again
		if( !client.mbInsideHook ){
			ImGui::Render(); 
		}
		
		// We were drawing frame for our remote connection, send the data				
		if( client.mbValidDrawFrame )
		{
			CmdDrawFrame* pNewDrawFrame = CreateCmdDrawDrame(ImGui::GetDrawData(), Cursor);
			client.mPendingFrameOut.Assign(pNewDrawFrame);
		}

		// Detect change to background settings by user, and forward them to server
		if( client.mBGSetting != client.mBGSettingSent )
		{
			CmdBackground* pCmdBackground	= netImguiNew<CmdBackground>();
			*pCmdBackground					= client.mBGSetting;
			client.mBGSettingSent			= client.mBGSetting;
			client.mPendingBackgroundOut.Assign(pCmdBackground);
		}

		// Restore display size, so we never lose original setting that may get updated after initial connection
		if( client.mbIsRemoteDrawing ) {			
			ImGui::GetIO().DisplaySize = client.mSavedDisplaySize;
		}
	}

	client.mbIsRemoteDrawing		= false;
	client.mbIsDrawing				= false;
	client.mbValidDrawFrame			= false;
}

//=================================================================================================
ImGuiContext* GetContext()
//=================================================================================================
{
	if (!gpClientInfo) return nullptr;

	Client::ClientInfo& client = *gpClientInfo;
	return client.mpContext;
}

//=================================================================================================
void SendDataTexture(ImTextureID textureId, void* pData, uint16_t width, uint16_t height, eTexFormat format)
//=================================================================================================
{
	if (!gpClientInfo) return;

	Client::ClientInfo& client				= *gpClientInfo;
	CmdTexture* pCmdTexture					= nullptr;

	// Makes sure even 32bits ImTextureID value are received properly as 64bits
	uint64_t texId64(0);
	static_assert(sizeof(uint64_t) <= sizeof(textureId), "ImTextureID is bigger than 64bits, CmdTexture::mTextureId needs to be updated to support it");
	reinterpret_cast<ImTextureID*>(&texId64)[0] = textureId;

	// Add/Update a texture
	if( pData != nullptr )
	{		
		uint32_t PixelDataSize				= GetTexture_BytePerImage(format, width, height);
		uint32_t SizeNeeded					= PixelDataSize + sizeof(CmdTexture);
		pCmdTexture							= netImguiSizedNew<CmdTexture>(SizeNeeded);

		pCmdTexture->mpTextureData.SetPtr(reinterpret_cast<uint8_t*>(&pCmdTexture[1]));
		memcpy(pCmdTexture->mpTextureData.Get(), pData, PixelDataSize);

		pCmdTexture->mHeader.mSize			= SizeNeeded;
		pCmdTexture->mWidth					= width;
		pCmdTexture->mHeight				= height;
		pCmdTexture->mTextureId				= texId64;
		pCmdTexture->mFormat				= format;
		pCmdTexture->mpTextureData.ToOffset();

		// Detects when user is sending the font texture
		ScopedImguiContext scopedCtx(client.mpContext ? client.mpContext : ImGui::GetCurrentContext());
		if( ImGui::GetIO().Fonts && ImGui::GetIO().Fonts->TexID == textureId )
		{
			client.mbFontUploaded		|= true;
			client.mpFontTextureData	= ImGui::GetIO().Fonts->TexPixelsAlpha8;
			client.mFontTextureID		= textureId;
		}		
	}
	// Texture to remove
	else
	{
		pCmdTexture							= netImguiNew<CmdTexture>();
		pCmdTexture->mTextureId				= texId64;		
		pCmdTexture->mWidth					= 0;
		pCmdTexture->mHeight				= 0;
		pCmdTexture->mFormat				= eTexFormat::kTexFmt_Invalid;
		pCmdTexture->mpTextureData.SetOff(0);
	}

	// In unlikely event of too many textures, wait for them to be processed 
	// (if connected) or Process them now (if not)
	while( (client.mTexturesPendingCreated - client.mTexturesPendingSent) >= static_cast<int32_t>(ArrayCount(client.mTexturesPending)) )
	{
		if( IsConnected() )
			std::this_thread::yield();
		else
			client.TextureProcessPending();
	}
	int32_t idx						= client.mTexturesPendingCreated.fetch_add(1) % static_cast<int32_t>(ArrayCount(client.mTexturesPending));
	client.mTexturesPending[idx]	= pCmdTexture;

	// If not connected to server yet, update all pending textures
	if( !IsConnected() )
		client.TextureProcessPending();
}

//=================================================================================================
void SetBackground(const ImVec4& bgColor)
//=================================================================================================
{
	if (!gpClientInfo) return;

	Client::ClientInfo& client			= *gpClientInfo;
	client.mBGSetting					= NetImgui::Internal::CmdBackground();
	client.mBGSetting.mClearColor[0]	= bgColor.x;
	client.mBGSetting.mClearColor[1]	= bgColor.y;
	client.mBGSetting.mClearColor[2]	= bgColor.z;
	client.mBGSetting.mClearColor[3]	= bgColor.w;
}

//=================================================================================================
void SetBackground(const ImVec4& bgColor, const ImVec4& textureTint )
//=================================================================================================
{
	if (!gpClientInfo) return;

	Client::ClientInfo& client			= *gpClientInfo;
	client.mBGSetting.mClearColor[0]	= bgColor.x;
	client.mBGSetting.mClearColor[1]	= bgColor.y;
	client.mBGSetting.mClearColor[2]	= bgColor.z;
	client.mBGSetting.mClearColor[3]	= bgColor.w;
	client.mBGSetting.mTextureTint[0]	= textureTint.x;
	client.mBGSetting.mTextureTint[1]	= textureTint.y;
	client.mBGSetting.mTextureTint[2]	= textureTint.z;
	client.mBGSetting.mTextureTint[3]	= textureTint.w;
	client.mBGSetting.mTextureId		= NetImgui::Internal::CmdBackground::kDefaultTexture;
}

//=================================================================================================
void SetBackground(const ImVec4& bgColor, const ImVec4& textureTint, ImTextureID bgTextureID)
//=================================================================================================
{
	if (!gpClientInfo) return;

	Client::ClientInfo& client			= *gpClientInfo;
	client.mBGSetting.mClearColor[0]	= bgColor.x;
	client.mBGSetting.mClearColor[1]	= bgColor.y;
	client.mBGSetting.mClearColor[2]	= bgColor.z;
	client.mBGSetting.mClearColor[3]	= bgColor.w;
	client.mBGSetting.mTextureTint[0]	= textureTint.x;
	client.mBGSetting.mTextureTint[1]	= textureTint.y;
	client.mBGSetting.mTextureTint[2]	= textureTint.z;
	client.mBGSetting.mTextureTint[3]	= textureTint.w;

	uint64_t texId64(0);
	reinterpret_cast<ImTextureID*>(&texId64)[0] = bgTextureID;
	client.mBGSetting.mTextureId		= texId64;
}

//=================================================================================================
bool Startup(void)
//=================================================================================================
{
	if (!gpClientInfo)
	{
		gpClientInfo = netImguiNew<Client::ClientInfo>();	
	}
	
	return Network::Startup();
}

//=================================================================================================
void Shutdown(bool bWait)
//=================================================================================================
{
	if (!gpClientInfo) return;
	
	Disconnect();
	while(bWait && gpClientInfo->IsActive() )
		std::this_thread::yield();
	Network::Shutdown();
	
	netImguiDeleteSafe(gpClientInfo);		
}


//=================================================================================================
ImGuiContext* CloneContext(ImGuiContext* pSourceContext)
//=================================================================================================
{
	// Create a context duplicate
	ScopedImguiContext scopedSourceCtx(pSourceContext);
	ImGuiContext* pContextClone = ImGui::CreateContext(ImGui::GetIO().Fonts);
	ImGuiIO& sourceIO = ImGui::GetIO();
	ImGuiStyle& sourceStyle = ImGui::GetStyle();
	{
		ScopedImguiContext scopedCloneCtx(pContextClone);
		ImGuiIO& newIO = ImGui::GetIO();
		ImGuiStyle& newStyle = ImGui::GetStyle();

		// Import the style/options settings of current context, into this one	
		memcpy(&newStyle, &sourceStyle, sizeof(newStyle));
		memcpy(&newIO, &sourceIO, sizeof(newIO));
		//memcpy(newIO.KeyMap, sourceIO.KeyMap, sizeof(newIO.KeyMap));
		newIO.InputQueueCharacters.Data = nullptr;
		newIO.InputQueueCharacters.Size = 0;
		newIO.InputQueueCharacters.Capacity = 0;
	}
	return pContextClone;
}

//=================================================================================================
uint8_t GetTexture_BitsPerPixel(eTexFormat eFormat)
//=================================================================================================
{
	switch(eFormat)
	{
	case eTexFormat::kTexFmtA8:			return 8*1;
	case eTexFormat::kTexFmtRGBA8:		return 8*4;
	case eTexFormat::kTexFmt_Invalid:	return 0;
	}
	return 0;
}

//=================================================================================================
uint32_t GetTexture_BytePerLine(eTexFormat eFormat, uint32_t pixelWidth)
//=================================================================================================
{		
	uint32_t bitsPerPixel = static_cast<uint32_t>(GetTexture_BitsPerPixel(eFormat));
	return pixelWidth * bitsPerPixel / 8;
	//Note: If adding support to BC compression format, have to take into account 4x4 size alignment
}
	
//=================================================================================================
uint32_t GetTexture_BytePerImage(eTexFormat eFormat, uint32_t pixelWidth, uint32_t pixelHeight)
//=================================================================================================
{
	return GetTexture_BytePerLine(eFormat, pixelWidth) * pixelHeight;
	//Note: If adding support to BC compression format, have to take into account 4x4 size alignement
}

//=================================================================================================
bool ProcessInputData(Client::ClientInfo& client)
//=================================================================================================
{
	CmdInput* pCmdInputNew	= client.mPendingInputIn.Release();
	bool hasNewInput		= pCmdInputNew != nullptr; 	
	CmdInput* pCmdInput		= hasNewInput ? pCmdInputNew : client.mpLastInput;
	ImGuiIO& io				= ImGui::GetIO();

	if (pCmdInput)
	{
		io.DisplaySize	= ImVec2(pCmdInput->mScreenSize[0], pCmdInput->mScreenSize[1]);
		io.MousePos		= ImVec2(pCmdInput->mMousePos[0], pCmdInput->mMousePos[1]);
		io.MouseWheel	= pCmdInput->mMouseWheelVert - client.mMouseWheelVertPrev;
		io.MouseWheelH	= pCmdInput->mMouseWheelHoriz - client.mMouseWheelHorizPrev;
		io.MouseDown[0] = pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkMouseBtnLeft);
		io.MouseDown[1] = pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkMouseBtnRight);
		io.MouseDown[2] = pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkMouseBtnMid);
		io.MouseDown[3] = pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkMouseBtnExtra1);
		io.MouseDown[4] = pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkMouseBtnExtra2);
		io.KeyShift		= pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkKeyboardShift);
		io.KeyCtrl		= pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkKeyboardCtrl);
		io.KeyAlt		= pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkKeyboardAlt);
		io.KeySuper		= pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkKeyboardSuper1) || pCmdInput->IsKeyDown(CmdInput::eVirtualKeys::vkKeyboardSuper2);
		//io.NavInputs // @sammyfreg TODO: Handle Gamepad

		memset(io.KeysDown, 0, sizeof(io.KeysDown));
		for (uint32_t i(0); i < ArrayCount(pCmdInput->mKeysDownMask) * 64; ++i)
			io.KeysDown[i] = (pCmdInput->mKeysDownMask[i / 64] & (static_cast<uint64_t>(1) << (i % 64))) != 0;

		// @sammyfreg TODO: Optimize this
		io.ClearInputCharacters();
		size_t keyCount(1);
		uint16_t character;
		client.mPendingKeyIn.ReadData(&character, keyCount);
		while (keyCount > 0)
		{
			io.AddInputCharacter(character);
			client.mPendingKeyIn.ReadData(&character, keyCount);
		}

		client.mMouseWheelVertPrev	= pCmdInput->mMouseWheelVert;
		client.mMouseWheelHorizPrev = pCmdInput->mMouseWheelHoriz;				
	}

	if( hasNewInput ){
		netImguiDeleteSafe(client.mpLastInput);
		client.mpLastInput		= pCmdInputNew;
	}
	return hasNewInput;
}

} // namespace NetImgui

#endif //NETIMGUI_ENABLED

#include "NetImgui_WarningReenable.h"
