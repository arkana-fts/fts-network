#include <string>
#include "fts-net.h"
#include "Logger.h"
#if defined(_WIN32)
#  pragma comment(lib, "Ws2_32.lib")
#  include<winsock2.h>
#endif

bool FTS::NetworkLibInit( int dbgLevel, std::ostream * out )
{
    Logger::DbgLevel( dbgLevel );
    Logger::LogFile(out);

#if defined(_WIN32)
    // And the windows sockets.
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        FTSMSGDBG("Winsock version {1}.{2} initialized ", 2, std::to_string(LOBYTE(wsaData.wVersion)), std::to_string(HIBYTE(wsaData.wVersion)));
#endif

    return true;
}
