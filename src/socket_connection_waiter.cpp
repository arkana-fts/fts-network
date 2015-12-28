/**
 * \file socket_connection_waiter.cpp
 * \author Pompei2
 * \date 03 Oct 2008
 * \brief This file implements the class that handles a lot of
 *        Network stuff at the server-side.
 **/

#include <thread>
#include <chrono>

#include "Logger.h"
#include "connection.h"
#include "TraditionalConnection.h"
#include "socket_connection_waiter.h"

#if defined(_WIN32)
#include <WinSock2.h>
using socklen_t = int;

inline void close( SOCKET s )
{
    closesocket( s );
}
#else
#  include <unistd.h> // close
#  include <string.h> // strerror
#endif

 /// The common NO error return value
#define ERR_OK  0

using namespace FTS;
using namespace std;

FTS::SocketConnectionWaiter::~SocketConnectionWaiter()
{
    close( m_listenSocket );
}

int FTS::SocketConnectionWaiter::init(std::uint16_t in_usPort, std::function<void( FTS::Connection* )> in_cb )
{
    m_cb = in_cb;
    m_port = in_usPort;
    SOCKADDR_IN serverAddress;

    // Choose our options.
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons((int)in_usPort);

    // Setup the listening socket.
    if((m_listenSocket = socket(AF_INET, SOCK_STREAM, 0 /*IPPROTO_TCP */ )) < 0) {
        FTSMSG("[ERROR] socket: "+string(strerror(errno)), MsgType::Error);
        return -1;
    }

    if(::bind(m_listenSocket, (sockaddr *) & serverAddress, sizeof(serverAddress)) < 0) {
        FTSMSG("[ERROR] socket bind: "+string(strerror(errno)), MsgType::Error);
        close(m_listenSocket);
        return -2;
    }

    // Set it to be nonblocking, so we can easily time the wait for a connection.
    TraditionalConnection::setSocketBlocking(m_listenSocket, false);

    if(listen(m_listenSocket, 100) < 0) {
        FTSMSG("[ERROR] socket listen: "+string(strerror(errno)), MsgType::Error);
        close(m_listenSocket);
        return -3;
    }

    FTSMSGDBG("Beginning to listen on port 0x"+toString(in_usPort, 0, ' ', std::ios::hex), 1);
    return ERR_OK;
}

void printSocketError()
{
    if( errno == EAGAIN || errno == EWOULDBLOCK ) {
        return;
    } else {
    #if defined(_WIN32)
        auto err = WSAGetLastError();
        if( err == WSAEWOULDBLOCK ) {
            return ;
        }
        FTSMSG( "[ERROR] socket accept: " + toString( err ), MsgType::Error );
    #else
        // Some error ... but continue waiting for a connection.
        FTSMSG( "[ERROR] socket accept: " + string( strerror( errno ) ), MsgType::Error );
    #endif
    }
}

bool FTS::SocketConnectionWaiter::waitForThenDoConnection(std::int64_t in_ulMaxWaitMillisec)
{
    auto startTime = std::chrono::steady_clock::now();
    // wait for connections a certain amount of time or infinitely.
    while(true) {
        // Nothing correct got in time, bye.
        auto nowTime = std::chrono::steady_clock::now();
        auto diffTime = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - startTime).count();
        if( diffTime >= in_ulMaxWaitMillisec )
            return false;

        SOCKADDR_IN clientAddress;
        socklen_t iClientAddressSize = sizeof( clientAddress );
        SOCKET connectSocket = accept( m_listenSocket, (sockaddr *) & clientAddress, &iClientAddressSize );
        if( connectSocket != -1 ) {
            // Yeah, we got someone !

            // Build up a class that will work this connection.
            Connection *pCon = new TraditionalConnection( connectSocket, clientAddress );
            m_cb( pCon );
            return true;
        } else {
            printSocketError();
            // Even if an error occured we don't leave.
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            continue;
        }
    }

    // Should never come here.
    return false;
}

