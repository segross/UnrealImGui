#include "NetImgui_Shared.h"

#if NETIMGUI_ENABLED && NETIMGUI_WINSOCKET_ENABLED
#include "NetImgui_WarningDisableStd.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

namespace NetImgui { namespace Internal { namespace Network 
{

struct SocketInfo
{
	SocketInfo(SOCKET socket) : mSocket(socket){}
	SOCKET mSocket;
};

bool Startup()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
		return false;
	
	return true;
}

void Shutdown()
{
	WSACleanup();
}

SocketInfo* Connect(const char* ServerHost, uint32_t ServerPort)
{
	SOCKET ConnectSocket = socket(AF_INET , SOCK_STREAM , 0);
	if(ConnectSocket == INVALID_SOCKET)
		return nullptr;
	
	char zPortName[32];
	addrinfo*	pResults	= nullptr;
	SocketInfo* pSocketInfo	= nullptr;
	sprintf_s(zPortName, "%i", ServerPort);
	getaddrinfo(ServerHost, zPortName, nullptr, &pResults);
	addrinfo*	pResultCur	= pResults;
	while( pResultCur && !pSocketInfo )
	{
		if( connect(ConnectSocket, pResultCur->ai_addr, static_cast<int>(pResultCur->ai_addrlen)) == 0 )
			pSocketInfo = netImguiNew<SocketInfo>(ConnectSocket);
				
		pResultCur = pResultCur->ai_next;
	}
	freeaddrinfo(pResults);
	return pSocketInfo;
}

SocketInfo* ListenStart(uint32_t ListenPort)
{
	SOCKET ListenSocket = INVALID_SOCKET;
	if( (ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != INVALID_SOCKET )
	{
		sockaddr_in server;
		server.sin_family		= AF_INET;
		server.sin_addr.s_addr	= INADDR_ANY;
		server.sin_port			= htons(static_cast<USHORT>(ListenPort));
		if(	bind(ListenSocket, reinterpret_cast<sockaddr*>(&server), sizeof(server)) != SOCKET_ERROR &&
			listen(ListenSocket, 0) != SOCKET_ERROR )
		{
			return netImguiNew<SocketInfo>(ListenSocket);
		}
		closesocket(ListenSocket);
	}
	return nullptr;
}

SocketInfo* ListenConnect(SocketInfo* ListenSocket)
{
	if( ListenSocket )
	{
		sockaddr ClientAddress;
		int	Size(sizeof(ClientAddress));
		SOCKET ServerSocket = accept(ListenSocket->mSocket, &ClientAddress, &Size) ;
		if (ServerSocket != INVALID_SOCKET)
		{
			return netImguiNew<SocketInfo>(ServerSocket);
		}
	}
	return nullptr;
}

void Disconnect(SocketInfo* pClientSocket)
{
	if( pClientSocket )
	{
		closesocket(pClientSocket->mSocket);
		netImguiDelete(pClientSocket);
	}
}

bool DataReceive(SocketInfo* pClientSocket, void* pDataIn, size_t Size)
{
	int resultRcv = recv(pClientSocket->mSocket, reinterpret_cast<char*>(pDataIn), static_cast<int>(Size), MSG_WAITALL);
	return resultRcv != SOCKET_ERROR && resultRcv > 0;
}

bool DataSend(SocketInfo* pClientSocket, void* pDataOut, size_t Size)
{
	int resultSend = send(pClientSocket->mSocket, reinterpret_cast<char*>(pDataOut), static_cast<int>(Size), 0);
	return resultSend != SOCKET_ERROR && resultSend > 0;
}

}}} // namespace NetImgui::Internal::Network

#include "NetImgui_WarningReenable.h"
#endif // #if NETIMGUI_ENABLED && NETIMGUI_WINSOCKET_ENABLED
