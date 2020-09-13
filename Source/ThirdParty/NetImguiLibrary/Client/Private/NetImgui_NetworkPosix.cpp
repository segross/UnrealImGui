#include "NetImgui_Shared.h"

#if NETIMGUI_ENABLED && NETIMGUI_POSIX_SOCKETS_ENABLED
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

namespace NetImgui { namespace Internal { namespace Network 
{

struct SocketInfo
{
	SocketInfo(int socket) : mSocket(socket){}
	int mSocket;
};

bool Startup()
{
	return true;
}

void Shutdown()
{
}

SocketInfo* Connect(const char* ServerHost, uint32_t ServerPort)
{
	int ConnectSocket = socket(AF_INET , SOCK_STREAM , 0 );
	if(ConnectSocket == -1)
		return nullptr;
	
	char zPortName[32];
	addrinfo*	pResults	= nullptr;
	SocketInfo* pSocketInfo	= nullptr;
	sprintf(zPortName, "%i", ServerPort);
	getaddrinfo(ServerHost, zPortName, nullptr, &pResults);
	addrinfo*	pResultCur	= pResults;
	while( pResultCur && !pSocketInfo)
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
	addrinfo hints;

	memset(&hints, 0, sizeof hints);
	hints.ai_family		= AF_UNSPEC;
	hints.ai_socktype	= SOCK_STREAM;
	hints.ai_flags		= AI_PASSIVE;

	addrinfo* addrInfo;
	getaddrinfo(nullptr, std::to_string(ListenPort).c_str(), &hints, &addrInfo);

	int ListenSocket = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
	if( ListenSocket != -1 )
	{
		if(	bind(ListenSocket, addrInfo->ai_addr, addrInfo->ai_addrlen) != -1 &&
			listen(ListenSocket, 0) != -1)
		{
			return netImguiNew<SocketInfo>(ListenSocket);
		}
		close(ListenSocket);
	}
	return nullptr;
}

SocketInfo* ListenConnect(SocketInfo* ListenSocket)
{
	if( ListenSocket )
	{
		sockaddr_storage ClientAddress;
		socklen_t Size(sizeof(ClientAddress));
		int ServerSocket = accept(ListenSocket->mSocket, (sockaddr*)&ClientAddress, &Size) ;
		if (ServerSocket != -1)
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
		close(pClientSocket->mSocket);
		netImguiDelete(pClientSocket);
	}
}

bool DataReceive(SocketInfo* pClientSocket, void* pDataIn, size_t Size)
{
	int resultRcv = recv(pClientSocket->mSocket, static_cast<char*>(pDataIn), static_cast<int>(Size), MSG_WAITALL);
	return resultRcv != -1 && resultRcv > 0;
}

bool DataSend(SocketInfo* pClientSocket, void* pDataOut, size_t Size)
{
	int resultSend = send(pClientSocket->mSocket, static_cast<char*>(pDataOut), static_cast<int>(Size), 0);
	return resultSend != -1 && resultSend > 0;
}

}}} // namespace NetImgui::Internal::Network

#endif // #if NETIMGUI_ENABLED && NETIMGUI_POSIX_SOCKETS_ENABLED

