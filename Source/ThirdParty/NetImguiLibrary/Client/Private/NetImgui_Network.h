#pragma once

namespace NetImgui { namespace Internal { namespace Network 
{

struct SocketInfo;

bool		Startup			(void);
void		Shutdown		(void);

SocketInfo* Connect			(const char* ServerHost, uint32_t ServerPort);	// Communication Socket expected to be blocking
SocketInfo* ListenConnect	(SocketInfo* ListenSocket);						// Communication Socket expected to be blocking
SocketInfo* ListenStart		(uint32_t ListenPort);							// Listening Socket expected to be non blocking
void		Disconnect		(SocketInfo* pClientSocket);

bool		DataReceive		(SocketInfo* pClientSocket, void* pDataIn, size_t Size);
bool		DataSend		(SocketInfo* pClientSocket, void* pDataOut, size_t Size);

}}} //namespace NetImgui::Internal::Network
