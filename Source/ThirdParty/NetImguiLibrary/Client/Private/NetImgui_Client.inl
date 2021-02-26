
#include "NetImgui_Network.h"

namespace NetImgui { namespace Internal { namespace Client {

void ClientTexture::Set( CmdTexture* pCmdTexture )
{
	netImguiDeleteSafe(mpCmdTexture);
	mpCmdTexture	= pCmdTexture;
	mbSent			= pCmdTexture == nullptr;
}

bool ClientTexture::IsValid()const
{
	return mpCmdTexture != nullptr;
}

bool ClientInfo::IsConnected()const
{
	return mpSocketComs.load() != nullptr;
}

bool ClientInfo::IsConnectPending()const
{
	return mpSocketPending.load() != nullptr || mpSocketListen.load() != nullptr;
}

bool ClientInfo::IsActive()const
{
	return IsConnected() || IsConnectPending() || mbDisconnectRequest;
}

void ClientInfo::KillSocketComs()
{
	Network::SocketInfo* pSocket = mpSocketPending.exchange(nullptr);
	if (pSocket)
	{
		NetImgui::Internal::Network::Disconnect(pSocket);
	}

	pSocket = mpSocketComs.exchange(nullptr);
	if (pSocket)
	{
		NetImgui::Internal::Network::Disconnect(pSocket);
	}
}

void ClientInfo::KillSocketListen()
{
	Network::SocketInfo* pSocket = mpSocketListen.exchange(nullptr);
	if (pSocket)
	{
		NetImgui::Internal::Network::Disconnect(pSocket);
	}
}

bool ClientInfo::IsContextOverriden()const
{
	return mSavedContextValues.mSavedContext;
}

}}} // namespace NetImgui::Internal::Client
