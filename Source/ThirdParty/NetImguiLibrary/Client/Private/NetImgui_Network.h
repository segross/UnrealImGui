#pragma once

namespace NetImgui { namespace Internal { namespace Network 
{

struct SocketInfo;

bool		Startup			(void);
void		Shutdown		(void);

SocketInfo* Connect			(const char* ServerHost, uint32_t ServerPort);
SocketInfo* ListenStart		(uint32_t ListenPort);
SocketInfo* ListenConnect	(SocketInfo* ListenSocket);
void		Disconnect		(SocketInfo* pClientSocket);

bool		DataReceive		(SocketInfo* pClientSocket, void* pDataIn, size_t Size);
bool		DataSend		(SocketInfo* pClientSocket, void* pDataOut, size_t Size);

}}} //namespace NetImgui::Internal::Network
