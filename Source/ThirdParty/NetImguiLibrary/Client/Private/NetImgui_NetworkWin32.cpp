#include "NetImgui_Shared.h"

#if NETIMGUI_ENABLED && NETIMGUI_WINSOCKET_ENABLED
#include "NetImgui_WarningDisableStd.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

#if defined(_MSC_VER)
#pragma comment(lib, "ws2_32")
#endif


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

inline void SetNonBlocking(SOCKET Socket, bool bIsNonBlocking)
{
	u_long IsNonBlocking = bIsNonBlocking;
	ioctlsocket(Socket, static_cast<long>(FIONBIO), &IsNonBlocking);
}

SocketInfo* Connect(const char* ServerHost, uint32_t ServerPort)
{
	SOCKET ClientSocket = socket(AF_INET , SOCK_STREAM , 0);
	if(ClientSocket == INVALID_SOCKET)
		return nullptr;
	
	char zPortName[32];
	addrinfo*	pResults	= nullptr;
	SocketInfo* pSocketInfo	= nullptr;
	sprintf_s(zPortName, "%i", ServerPort);
	getaddrinfo(ServerHost, zPortName, nullptr, &pResults);
	addrinfo*	pResultCur	= pResults;
	while( pResultCur && !pSocketInfo )
	{
		if( connect(ClientSocket, pResultCur->ai_addr, static_cast<int>(pResultCur->ai_addrlen)) == 0 )
		{	
			SetNonBlocking(ClientSocket, false);
			pSocketInfo = netImguiNew<SocketInfo>(ClientSocket);
		}		
		pResultCur = pResultCur->ai_next;
	}
	freeaddrinfo(pResults);
	if( !pSocketInfo )
	{
		closesocket(ClientSocket);
	}
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
		
		constexpr BOOL ReUseAdrValue(true);
		setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&ReUseAdrValue), sizeof(ReUseAdrValue));
		if(	bind(ListenSocket, reinterpret_cast<sockaddr*>(&server), sizeof(server)) != SOCKET_ERROR &&
			listen(ListenSocket, 0) != SOCKET_ERROR )
		{
			SetNonBlocking(ListenSocket, true);
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
		SOCKET ClientSocket = accept(ListenSocket->mSocket, &ClientAddress, &Size) ;
		if (ClientSocket != INVALID_SOCKET)
		{
		#if 0 // @sammyfreg : No timeout useful when debugging, to keep connection alive while code breakpoint
			static constexpr DWORD	kComsTimeoutMs	= 2000;
			setsockopt(ClientSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&kComsTimeoutMs), sizeof(kComsTimeoutMs));
			setsockopt(ClientSocket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&kComsTimeoutMs), sizeof(kComsTimeoutMs));
		#endif
			SetNonBlocking(ClientSocket, false);
			return netImguiNew<SocketInfo>(ClientSocket);
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
	return resultRcv != SOCKET_ERROR && static_cast<int>(Size) == resultRcv;
}

bool DataSend(SocketInfo* pClientSocket, void* pDataOut, size_t Size)
{
	int resultSend = send(pClientSocket->mSocket, reinterpret_cast<char*>(pDataOut), static_cast<int>(Size), 0);
	return resultSend != SOCKET_ERROR && static_cast<int>(Size) == resultSend;
}

}}} // namespace NetImgui::Internal::Network

#include "NetImgui_WarningReenable.h"
#else

// Prevents Linker warning LNK4221 in Visual Studio (This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library)
extern int sSuppresstLNK4221_NetImgui_NetworkWin23; 
int sSuppresstLNK4221_NetImgui_NetworkWin23(0);

#endif // #if NETIMGUI_ENABLED && NETIMGUI_WINSOCKET_ENABLED
