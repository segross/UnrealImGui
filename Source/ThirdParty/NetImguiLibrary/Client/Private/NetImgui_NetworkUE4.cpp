#include "NetImgui_Shared.h"

#if NETIMGUI_ENABLED && defined(__UNREAL__)

#include "CoreMinimal.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

namespace NetImgui { namespace Internal { namespace Network 
{

struct SocketInfo
{
	SocketInfo(FSocket* pSocket) : mpSocket(pSocket) {}
	~SocketInfo() { Close(); }
	void Close()
	{
		if(mpSocket )
		{
			mpSocket->Close();
			ISocketSubsystem::Get()->DestroySocket(mpSocket);
			mpSocket = nullptr;
		}
	}
	FSocket* mpSocket;
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
	SocketInfo* pSocketInfo					= nullptr;
	ISocketSubsystem* SocketSubSystem		= ISocketSubsystem::Get();
	auto ResolveInfo						= SocketSubSystem->GetHostByName(ServerHost);
	while( !ResolveInfo->IsComplete() )
		FPlatformProcess::Sleep(0.1);
	
	if (ResolveInfo->GetErrorCode() == 0)
	{
		TSharedRef<FInternetAddr> IpAddress	= ResolveInfo->GetResolvedAddress().Clone();
		IpAddress->SetPort(ServerPort);		
		if (IpAddress->IsValid())
		{
			FSocket* pNewSocket				= SocketSubSystem->CreateSocket(NAME_Stream, "netImgui", IpAddress->GetProtocolType());
			if (pNewSocket)
			{
				pNewSocket->SetNonBlocking(false);
				pSocketInfo = netImguiNew<SocketInfo>(pNewSocket);
				if (pNewSocket->Connect(IpAddress.Get()))
				{
					return pSocketInfo;
				}
			}
		}		
	}
	netImguiDelete(pSocketInfo);
	return nullptr;
}

SocketInfo* ListenStart(uint32_t ListenPort)
{
	ISocketSubsystem* PlatformSocketSub = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	TSharedRef<FInternetAddr> IpAddress = PlatformSocketSub->CreateInternetAddr();
	IpAddress->SetLoopbackAddress();
	IpAddress->SetPort(ListenPort);
	FSocket* pNewListenSocket			= PlatformSocketSub->CreateSocket(NAME_Stream, "netImguiListen", IpAddress->GetProtocolType());
	SocketInfo* pListenSocketInfo		= netImguiNew<SocketInfo>(pNewListenSocket);
	if (pNewListenSocket->Bind(*IpAddress))
	{
		pNewListenSocket->SetNonBlocking(true);
		if (pNewListenSocket->Listen(1))
		{
			return pListenSocketInfo;
		}
	}

	netImguiDelete(pListenSocketInfo);
	return nullptr;
}

SocketInfo* ListenConnect(SocketInfo* pListenSocket)
{
	if (pListenSocket)
	{
		FSocket* pNewSocket = pListenSocket->mpSocket->Accept(FString("netImgui"));
		if( pNewSocket )
		{
			pNewSocket->SetNonBlocking(false);
			SocketInfo* pSocketInfo = netImguiNew<SocketInfo>(pNewSocket);
			return pSocketInfo;
		}
	}
	return nullptr;
}

void Disconnect(SocketInfo* pClientSocket)
{
	netImguiDelete(pClientSocket);	
}

bool DataReceive(SocketInfo* pClientSocket, void* pDataIn, size_t Size)
{
	int32 sizeRcv(0);
	bool bResult = pClientSocket->mpSocket->Recv(reinterpret_cast<uint8*>(pDataIn), Size, sizeRcv, ESocketReceiveFlags::WaitAll);
	return bResult && static_cast<int32>(Size) == sizeRcv;
}

bool DataSend(SocketInfo* pClientSocket, void* pDataOut, size_t Size)
{
	int32 sizeSent(0);
	bool bResult = pClientSocket->mpSocket->Send(reinterpret_cast<uint8*>(pDataOut), Size, sizeSent);
	return bResult && static_cast<int32>(Size) == sizeSent;
}

}}} // namespace NetImgui::Internal::Network

#else

// Prevents Linker warning LNK4221 in Visual Studio (This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library)
extern int sSuppresstLNK4221_NetImgui_NetworkUE4; 
int sSuppresstLNK4221_NetImgui_NetworkUE4(0);


#endif // #if NETIMGUI_ENABLED && defined(__UNREAL__)
