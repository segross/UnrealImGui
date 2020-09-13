#include "NetImgui_Shared.h"

#if NETIMGUI_ENABLED
#include "NetImgui_WarningDisable.h"
#include "NetImgui_Client.h"
#include "NetImgui_Network.h"
#include "NetImgui_CmdPackets.h"

namespace NetImgui { namespace Internal { namespace Client 
{

//=================================================================================================
// CLIENT INFO Constructor
//=================================================================================================
ClientInfo::ClientInfo() 
: mTexturesPendingCount(0) 
{ 
	memset(mTexturesPending, 0, sizeof(mTexturesPending)); 
}

//=================================================================================================
// COMMUNICATIONS INITIALIZE
// Initialize a new connection to a RemoteImgui server
//=================================================================================================
bool Communications_Initialize(ClientInfo& client)
{
	CmdVersion cmdVersionSend;
	CmdVersion cmdVersionRcv;	
	StringCopy(cmdVersionSend.mClientName, client.mName);
	bool bResultSend =		Network::DataSend(client.mpSocket, &cmdVersionSend, cmdVersionSend.mHeader.mSize);
	bool bResultRcv	=		Network::DataReceive(client.mpSocket, &cmdVersionRcv, sizeof(cmdVersionRcv));		
	client.mbConnected =	bResultRcv && bResultSend && 
							cmdVersionRcv.mHeader.mType == CmdHeader::eCommands::Version && 
							cmdVersionRcv.mVersion == CmdVersion::eVersion::_Current;
	client.mbConnectRequest = false;
	if( client.mbConnected )
	{
		client.mbHasTextureUpdate = true;
		for(auto& texture : client.mTextures)
			texture.mbSent = false;

		return true;
	}
	return false;
}

//=================================================================================================
// INCOM: INPUT
// Receive new keyboard/mouse/screen resolution input to pass on to dearImgui
//=================================================================================================
void Communications_Incoming_Input(ClientInfo& client, uint8_t*& pCmdData)
{
	if( pCmdData )
	{
		auto pCmdInput	= reinterpret_cast<CmdInput*>(pCmdData);
		pCmdData		= nullptr; // Take ownership of the data, prevent Free
		size_t keyCount(pCmdInput->mKeyCharCount);
		client.mPendingKeyIn.AddData(pCmdInput->mKeyChars, keyCount);
		client.mPendingInputIn.Assign(pCmdInput);	
	}
}

//=================================================================================================
// OUTCOM: TEXTURE
// Transmit all pending new/updated texture
//=================================================================================================
bool Communications_Outgoing_Textures(ClientInfo& client)
{	
	bool bSuccess(true);
	client.TextureProcessPending();
	if( client.mbHasTextureUpdate )
	{
		for(auto& cmdTexture : client.mTextures)
		{
			if( !cmdTexture.mbSent && cmdTexture.mpCmdTexture )
			{
				bSuccess			&= Network::DataSend(client.mpSocket, cmdTexture.mpCmdTexture, cmdTexture.mpCmdTexture->mHeader.mSize);
				cmdTexture.mbSent	= bSuccess;
				if( cmdTexture.mbSent && cmdTexture.mpCmdTexture->mFormat == eTexFormat::kTexFmt_Invalid )
					netImguiDeleteSafe(cmdTexture.mpCmdTexture);					
			}
		}
		client.mbHasTextureUpdate = !bSuccess;
	}
	return bSuccess;
}

//=================================================================================================
// OUTCOM: FRAME
// Transmit a new dearImgui frame to render
//=================================================================================================
bool Communications_Outgoing_Frame(ClientInfo& client)
{
	bool bSuccess(true);
	CmdDrawFrame* pPendingDrawFrame = client.mPendingFrameOut.Release();
	if( pPendingDrawFrame )
	{
		bSuccess = Network::DataSend(client.mpSocket, pPendingDrawFrame, pPendingDrawFrame->mHeader.mSize);
		netImguiDeleteSafe(pPendingDrawFrame);
	}
	return bSuccess;
}

//=================================================================================================
// OUTCOM: DISCONNECT
// Signal that we will be disconnecting
//=================================================================================================
bool Communications_Outgoing_Disconnect(ClientInfo& client)
{
	if( client.mbDisconnectRequest )
	{
		CmdDisconnect cmdDisconnect;
		Network::DataSend(client.mpSocket, &cmdDisconnect, cmdDisconnect.mHeader.mSize);
		return false;
	}
	return true;
}

//=================================================================================================
// OUTCOM: PING
// Signal end of outgoing communications and still alive
//=================================================================================================
bool Communications_Outgoing_Ping(ClientInfo& client)
{
	CmdPing cmdPing;
	return Network::DataSend(client.mpSocket, &cmdPing, cmdPing.mHeader.mSize);		
}

//=================================================================================================
// INCOMING COMMUNICATIONS
//=================================================================================================
bool Communications_Incoming(ClientInfo& client)
{
	bool bOk(true);
	bool bPingReceived(false);
	while( bOk && !bPingReceived )
	{
		CmdHeader cmdHeader;
		uint8_t* pCmdData	= nullptr;
		bOk					= Network::DataReceive(client.mpSocket, &cmdHeader, sizeof(cmdHeader));
		if( bOk && cmdHeader.mSize > sizeof(CmdHeader) )
		{
			pCmdData								= netImguiSizedNew<uint8_t>(cmdHeader.mSize);
			*reinterpret_cast<CmdHeader*>(pCmdData) = cmdHeader;
			bOk										= Network::DataReceive(client.mpSocket, &pCmdData[sizeof(cmdHeader)], cmdHeader.mSize-sizeof(cmdHeader));	
		}

		if( bOk )
		{
			switch( cmdHeader.mType )
			{
			case CmdHeader::eCommands::Ping:		bPingReceived = true; break;
			case CmdHeader::eCommands::Disconnect:	bOk = false; break;
			case CmdHeader::eCommands::Input:		Communications_Incoming_Input(client, pCmdData); break;			
			// Commands not received in main loop, by Client
			case CmdHeader::eCommands::Invalid:
			case CmdHeader::eCommands::Version:
			case CmdHeader::eCommands::Texture:
			case CmdHeader::eCommands::DrawFrame:	break;
			}
		}		
		netImguiDeleteSafe(pCmdData);
	}
	return bOk;
}


//=================================================================================================
// OUTGOING COMMUNICATIONS
//=================================================================================================
bool Communications_Outgoing(ClientInfo& client)
{		
	bool bSuccess(true);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Textures(client);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Frame(client);	
	if( bSuccess )
		bSuccess = Communications_Outgoing_Disconnect(client);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Ping(client); // Always finish with a ping

	return bSuccess;
}

//=================================================================================================
// COMMUNICATIONS THREAD 
//=================================================================================================
void CommunicationsClient(void* pClientVoid)
{	
	ClientInfo* pClient = reinterpret_cast<ClientInfo*>(pClientVoid);
	Communications_Initialize(*pClient);
	bool bConnected(pClient->mbConnected);
	while( bConnected )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(8));
		bConnected	= Communications_Outgoing(*pClient) && Communications_Incoming(*pClient);		
	}
	Network::Disconnect(pClient->mpSocket);
	pClient->mpSocket				= nullptr;
	pClient->mbDisconnectRequest	= false;
	pClient->mbConnected			= false;		
}

//=================================================================================================
// COMMUNICATIONS WAIT THREAD 
//=================================================================================================
void CommunicationsHost(void* pClientVoid)
{
	ClientInfo* pClient					= reinterpret_cast<ClientInfo*>(pClientVoid);	
	Network::SocketInfo* pListenSocket	= pClient->mpSocket;
	pClient->mpSocket					= nullptr;

	while( !pClient->mbDisconnectRequest )
	{		
		pClient->mbConnectRequest		= true;
		pClient->mpSocket				= Network::ListenConnect( pListenSocket );
		if( pClient->mpSocket )
		{
			Communications_Initialize(*pClient);
			bool bConnected				= pClient->mbConnected;
			pClient->mbConnectRequest	= !bConnected;
			while (bConnected)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(8));
				bConnected				= Communications_Outgoing(*pClient) && Communications_Incoming(*pClient);
			}
			Network::Disconnect(pClient->mpSocket);
			pClient->mpSocket			= nullptr;
			pClient->mbConnected		= false;
		}			
	}
	Network::Disconnect(pListenSocket);	
	pListenSocket						= nullptr;
	pClient->mbDisconnectRequest		= false;
}

//=================================================================================================
//
//=================================================================================================
void ClientInfo::TextureProcessPending()
{
	mbHasTextureUpdate |= mTexturesPendingCount > 0;
	while( mTexturesPendingCount > 0 )
	{
		int32_t count				= mTexturesPendingCount.fetch_sub(1);
		CmdTexture* pCmdTexture		= mTexturesPending[count-1];
		mTexturesPending[count-1]	= nullptr;
		if( pCmdTexture )
		{
			// Find the TextureId from our list (or free slot)
			int texIdx		= 0;
			int texFreeSlot	= static_cast<int>(mTextures.size());
			while( texIdx < mTextures.size() && ( !mTextures[texIdx].IsValid() || mTextures[texIdx].mpCmdTexture->mTextureId != pCmdTexture->mTextureId) )
			{
				texFreeSlot = !mTextures[texIdx].IsValid() ? texIdx : texFreeSlot;
				++texIdx;
			}

			if( texIdx == mTextures.size() )
				texIdx = texFreeSlot;
			if( texIdx == mTextures.size() )
				mTextures.push_back(ClientTexture());

			mTextures[texIdx].Set( pCmdTexture );
			mTextures[texIdx].mbSent = false;
		}
	}
}

}}} // namespace NetImgui::Internal::Client

#include "NetImgui_WarningReenable.h"
#endif //#if NETIMGUI_ENABLED
