/**
 * \file connection.cpp
 * \author Pompei2
 * \date 11 May 2007
 * \brief This file implements the class that represents a network
 *        connection that can send packets.
 **/

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cassert>
#include <vector>
#include <ostream>
#include <iostream>
#include <cstring>

#include "connection.h"
#include "packet.h"

#include "logging/MsgType.h"
#include "utilities/DataContainer.h"

#if !defined( _WIN32 )
#  include <unistd.h>
#  include <poll.h>
#  include <sys/select.h>
#  include <resolv.h>
#  include <netdb.h>
#  include <fcntl.h>
#  include <arpa/inet.h>
#else
// Gotta work this around with a function cuz a define would be too risked.
inline void close(SOCKET s)
{
    closesocket(s);
    return;
}
#if defined(_DEBUG)
#  define DEBUG
#endif

#endif


template<typename T>
void pack_param(std::vector<std::string>& pack, T first )
{
    pack.push_back( first );
}

template<typename T, typename... Ts>
void pack_param(std::vector<std::string>& pack, T first, Ts... params )
{
    pack.push_back( first );
    pack_param( pack, params... );
}

template<typename... Ts>
void FTSMSGDBG( std::string in_Msg, int in_iDbgLv, Ts... params )
{
    std::vector<std::string> args;
    pack_param( args, params... );
    int i = 1;
    for( auto p : args ) {
        std::string fmt = "{" + toString( i ) + "}";
        auto pos = in_Msg.find( fmt );
        if( pos != std::string::npos ) {
            in_Msg.replace( pos, fmt.size(), p );
        }
    }
    FTSMSGDBG( in_Msg, in_iDbgLv);
}
template<>
void FTSMSGDBG( std::string in_Msg, int in_iDbgLv)
{
    std::cout << in_Msg;
}

template<typename... Ts>
void FTS18N( std::string in_Msg, FTS::MsgType::Enum in_Gravity , Ts... params )
{
    std::vector<std::string> args;
    pack_param( args, params... );
    int i = 1;
    for( auto p : args ) {
        std::string fmt = "{" + toString( i ) + "}";
        auto pos = in_Msg.find( fmt );
        if( pos != std::string::npos ) {
            in_Msg.replace( pos, fmt.size(), p );
        }
    }
    FTS18N( in_Msg, in_Gravity);
}

template<>
void FTS18N( std::string in_Msg, FTS::MsgType::Enum in_Gravity )
{
    std::cout << in_Msg;
}

using namespace FTS;

#if defined(D_COMPILES_SERVER)
#include "server_log.h"
void addRecvPacketStat( Packet* p )
{
    dynamic_cast< FTSSrv2::ServerLogger * >(FTS::Logger::getSingletonPtr())->statAddRecvPacket( p->getType() );
}
void addSendPacketStat( Packet* p )
{
    dynamic_cast< FTSSrv2::ServerLogger * >(FTS::Logger::getSingletonPtr())->statAddSendPacket( p->getType() );
}
#else
void addRecvPacketStat( Packet* p ) {}
void addSendPacketStat( Packet* p ) {}
#endif


#if defined(DEBUG) 
#define D_DEBUG_QUEUE
#endif
/// TODO: Add better logging of what travels trough the net maybe ?
#define NETLOG 1
#if NETLOG && defined(DEBUG)
void netlog(const std::string &in_s)
{
    FTSMSGDBG(in_s+"\n", 5);
}
std::string toHexString( const char* buf, int len )
{
    std::stringstream out;
    out.width( 2 );
    out.fill( ' ' );
    out.flags( std::ios::hex );
    for( int i = 0; i < len; ++i ) {
        out << *buf++;
    }
    return out.str();
}

void netlog2(const std::string &in_s, const void* id, uint32_t in_uiLen, const char *in_pBuf)
{
    const std::string sHex = toHexString(in_pBuf, in_uiLen);
    const std::string sIdent = toString(reinterpret_cast<const uint32_t>(id), 4, '0', std::ios_base::hex );
    std::string sMsg = "<" + sIdent + ">" + in_s + ": " + toString( in_uiLen ) + " Bytes: " + sHex + " (\"";
    for(uint32_t i = 0 ; i < in_uiLen ; i++) {
        // Replace control characters by a space for output.
        if(in_pBuf[i] < 32) {
            sMsg += " ";
        } else {
            sMsg += toString(in_pBuf[i]);
        }
    }

    sMsg += "\"";
    netlog(sMsg);
}
#else
#  define netlog(a)
#  define netlog2(a, b, c, d)
#endif


/// Retrieves the packet in front of the queue or the first packet with a special ID.
/** This takes out either the packet that is in front of the message queue (if \a in_req
 *  is DSRV_MSG_NONE) or the first packet whose request id is \a in_req and
 *  returns it to the caller. If there is no message in the queue or no message with
 *  the request id being \a in_req, it just returns NULL.
 *
 * \return If successful:    A pointer to the packet.
 * \return If queue is empty or no request was found: NULL
 *
 * \note The user has to free the returned value !
 *
 * \author Pompei2
 */
Packet *FTS::Connection::getFirstPacketFromQueue( master_request_t in_req )
{
    if( m_lpPacketQueue.empty() ) {
        return nullptr;
    }

    // If we found none with that req_id, return NULL.
    Packet *p = nullptr;

    // Just get the first one ?
    if( in_req == DSRV_MSG_NONE ) {
        // Just get the first packet of the queue.
        p = m_lpPacketQueue.front();
        m_lpPacketQueue.pop_front();
    } else {
        // Search the list for the first packet with the corresponding request id.
        auto i = std::find_if( std::begin( m_lpPacketQueue ), std::end( m_lpPacketQueue ), [in_req] ( Packet* packet ) {
            return packet->getType() == in_req ;
        } );

        if( i != std::end( m_lpPacketQueue ) ) {
            m_lpPacketQueue.erase( i );
            p = *i;
        }
    }

#ifdef D_DEBUG_QUEUE
    if( p != nullptr ) {
        FTSMSGDBG("Recv packet from queue with ID 0x{1}, payload len: {2}", 4, toString(p->getType(), -1, ' ', std::ios::hex), toString(p->getPayloadLen()));
        std::string s = "Queue is now: (len:"+toString(m_lpPacketQueue.size())+")";
        for(auto pPack : m_lpPacketQueue) {
            s += "(0x" + toString(pPack->getType(), -1, ' ', std::ios::hex) + "," + toString(pPack->getPayloadLen()) + ")";
        }
        s += "End.";
        FTSMSGDBG(s, 4);
    }
#endif

    return p;
}

/// Adds a packet to the queue.
/** This first adds a packet at the end of the queue, then checks if the queue is
 *  too big. If it is, it drops out the first packet in the front of the queue.
 *  Also displays debugging messages about what it does.
 *
 * \author Pompei2
 */
void FTS::Connection::queuePacket(Packet *in_pPacket)
{
    if(!in_pPacket)
        return ;

    m_lpPacketQueue.push_back(in_pPacket);

    // Don't make the queue too big.
    while(m_lpPacketQueue.size() > FTSC_MAX_QUEUE_LEN) {
        Packet *pPack = m_lpPacketQueue.front();
#ifdef D_DEBUG_QUEUE
        FTSMSGDBG("Queue full, dropping packet with ID 0x{1}, payload len: {2}",4,
                  toString(pPack->getType(), -1, ' ', std::ios::hex), toString(pPack->getPayloadLen()));
#endif
        m_lpPacketQueue.pop_front();
        delete pPack;
    }

#ifdef D_DEBUG_QUEUE
    FTSMSGDBG("Queued packet with ID 0x{1}, payload len: {2}", 4,
              toString(in_pPacket->getType(),-1,' ',std::ios::hex), toString(in_pPacket->getPayloadLen()));
    std::string s = "Queue is now: (len:"+toString(m_lpPacketQueue.size())+")";
    for(auto pPack : m_lpPacketQueue) {
        s += "(0x" + toString(pPack->getType(), -1, ' ', std::ios::hex) + "," + toString(pPack->getPayloadLen()) + ")";
    }
    s += "End.";
    FTSMSGDBG(s, 4);
#endif
}

/// Creates the connection object and connect.
/** This creates the connection object and tries to connect to the specified
 *  server. You can check if the connection succeeded by calling the
 *  isConnected method.
 *
 * \param in_sName The name of the machine to connect to. This may be a name like
 *                 srv.bla.org or an IP-address like 127.0.0.1
 * \param in_usPort The port to connect to.
 * \param in_nTimeout The maximum number of milliseconds (1/1000 seconds)
 *                    to wait for a connection.
 *
 * \author Klaus Beyer 
 *
 * \note modified by Pompei2
 */
FTS::TraditionalConnection::TraditionalConnection(const std::string &in_sName, uint16_t in_usPort, uint64_t in_ulTimeoutInMillisec)
    : m_bConnected(false)
    , m_sock(0)
{
    setMaxWaitMillisec( in_ulTimeoutInMillisec );
    memset( &m_saCounterpart, 0, sizeof( m_saCounterpart ) );
    connectByName(in_sName, in_usPort);
}

/*! ctor. Uses an existing connection described by the 2nd parameter.
 *
 * \author Klaus.Beyer 
 *
 * \param[in] in_sock the socket to use for the connection
 * \param[in] in_sa   address of counterpart to which the connection goes.
 *
 */
FTS::TraditionalConnection::TraditionalConnection(SOCKET in_sock, SOCKADDR_IN in_sa)
    : m_bConnected(true)
    , m_sock(in_sock)
    , m_saCounterpart(in_sa)
{
}

/// Default destructor
/** Closes the connection.
 *
 * \author Pompei2
 */
FTS::TraditionalConnection::~TraditionalConnection()
{
    this->disconnect();
}


/** Returns an already received packet.
 *
 * \author Klaus Beyer
 */
Packet *FTS::TraditionalConnection::getReceivedPacketIfAny()
{
    return getFirstPacketFromQueue();
}

/// Check if i'm connected.
/** This checks if this connection is currently up or down.
 *
 * \return true if this connection is up, false if it's down.
 *
 * \note This function doesn't currently check if the connection
 *       is up right now, it just looks at the state the connection
 *       was during the last send/recv operation.
 *
 * \author Pompei2
 */
bool FTS::TraditionalConnection::isConnected()
{
    return m_bConnected;
}

/// Closes the connection.
/** This safely closes the connection with the counterpart by closing the socket.
 *
 * \note You don't need to call this, as it gets called by the destructor.
 *
 * \author Pompei2
 */
void FTS::TraditionalConnection::disconnect()
{
    if(m_bConnected) {
        close(m_sock);
        m_bConnected = false;
    }
    // We need to check empty the queue ourselves.
    if(!m_lpPacketQueue.empty()) {
#if defined(D_DEBUG_QUEUE)
        FTSMSGDBG( "There are still {1} packets in the queue left.", 4, toString( m_lpPacketQueue.size() ) );
#endif
        for( auto p : m_lpPacketQueue ) {
            delete p;
        }
    }
}

/// Return the IP address of the counterpart.
/** This returns a string containing the counterpart's IPv4 address
 *  in the format "xxx.xxx.xxx.xxx".
 *
 * \return a string containing counterpart's IPv4 address.
 *
 * \author Pompei2
 */
std::string FTS::TraditionalConnection::getCounterpartIP() const
{
    return inet_ntoa(m_saCounterpart.sin_addr);
}

/// Connects to another pc by it's name.
/** This resolves the name to an IPv4 address and then connects to it.
 *  If we are already connected, it first closes the old connection.
 *
 * \param in_sName    The name of the computer to connect to.
 * \param in_iPort    The port you want to use for the connection.
 *
 * \return If successful: OK
 * \return If failed:     Error code
 *
 * \author Pompei2
 */
FTSC_ERR FTS::TraditionalConnection::connectByName( std::string in_sName, uint16_t in_usPort)
{
    if(this->isConnected()) {
        this->disconnect();
    }

    hostent *serverInfo = nullptr;

    // Setup the connection socket.
#if WINDOOF
    if( (m_sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP )) == INVALID_SOCKET ) {
#else
    if((m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
#endif
        FTS18N( "Net_TCPIP_mksock", MsgType::Error, std::string(strerror( errno )), toString( errno ) );
        return FTSC_ERR::SOCKET;
    }

    // Get some information we need to connect to the server.
    if( nullptr == (serverInfo = gethostbyname( in_sName.c_str() )) ) {
        switch( h_errno ) {
            case -1:
                FTS18N( "Net_TCPIP_hostname", MsgType::Error, in_sName, std::string(strerror( errno )), toString( errno ) );
                break;
            default:
                FTS18N( "Net_TCPIP_hostname", MsgType::Error, in_sName, "Unknown hostname", toString( h_errno ) );
                break;
        }
        close( m_sock );
        return FTSC_ERR::HOST_NAME;
    }

    // Prepare to connect.
    m_saCounterpart.sin_family = serverInfo->h_addrtype;
    memcpy( (char *) &m_saCounterpart.sin_addr.s_addr, serverInfo->h_addr_list[0], serverInfo->h_length );
    m_saCounterpart.sin_port = htons( in_usPort );

    // Set the socket non-blocking so we can cancel it if it can't connect.
    this->setSocketBlocking(m_sock, false);

    auto startTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    // Try to connect to the server.
    do {
        int iRet = connect(m_sock, (struct sockaddr *)&m_saCounterpart, sizeof(m_saCounterpart));

        // It was successful.
        if(iRet == 0) {
            FTSMSGDBG( "Successful connected.\n", 0 );
            m_bConnected = true;
            return FTSC_ERR::OK;
#if WINDOOF
        } else if(WSAGetLastError() == WSAEISCONN) {
#else
        } else if(errno == EISCONN) {
#endif
            // Already connected.
            m_bConnected = true;
            return FTSC_ERR::OK;

#if WINDOOF
#else
        } else if(errno == EINPROGRESS) {
            // Need to wait for the socket to be available.
            int serr = 0;
            pollfd pfd;
            pfd.fd = m_sock;
            pfd.events = 0 | POLLOUT;
            pfd.revents = 0;

            // Wait an amount of time or wait infinitely
            if(m_maxWaitMillisec == ((uint64_t)(-1)))
                serr = ::poll( &pfd, 1, -1 );
            else
                serr = ::poll( &pfd, 1, (int)(m_maxWaitMillisec) );
            if(serr == SOCKET_ERROR) {
                FTS18N("Net_TCPIP_select", MsgType::Error, std::string(strerror(errno)), toString(errno));
                close(m_sock);
                return FTSC_ERR::SOCKET;
            }

            int result = 0;
            socklen_t len = sizeof(int);
            getsockopt(m_sock, SOL_SOCKET, SO_ERROR, &result, &len);
            // Connection succeeded.
            if(result == 0 || result == EISCONN) {
                m_bConnected = true;
                return FTSC_ERR::OK;
            } else {
                FTS18N("Net_TCPIP_connect", MsgType::Error, in_sName, toString(in_usPort), std::string(strerror(result)), toString(result));
                close(m_sock);
                return FTSC_ERR::SOCKET;
            }
#endif

            // There was another error then the retry/in progress/busy error.
#if WINDOOF
        } else if(WSAGetLastError() != WSAEWOULDBLOCK &&
                  WSAGetLastError() != WSAEALREADY &&
                  WSAGetLastError() != WSAEINVAL) {
                      FTS18N("Net_TCPIP_connect", MsgType::Error, in_sName, toString(in_usPort),
                   "Address not found (maybe you're not connected to the internet)",
                   toString(WSAGetLastError()));
#else
        } else if(/*errno != EINPROGRESS &&*/
                  errno != EALREADY &&
                  errno != EAGAIN) {
            FTS18N( "Net_TCPIP_connect", MsgType::Error, in_sName, toString( in_usPort ), std::string( strerror( errno )), toString( errno ) );
#endif
            close(m_sock);
            return FTSC_ERR::NOT_CONNECTED;
        }

        // Retry as long as we have time to.
        currentTime = std::chrono::steady_clock::now();
    } while( std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() < m_maxWaitMillisec);

#if WINDOOF
    FTS18N( "Net_TCPIP_connect_to", MsgType::Error, in_sName, toString(in_usPort), "Timed out (maybe the counterpart is down)", toString(WSAGetLastError( )) );
#else
    FTS18N( "Net_TCPIP_connect_to", MsgType::Error, in_sName, toString(in_usPort), std::string(strerror( errno )), toString(errno) );
#endif
    return FTSC_ERR::TIMEOUT;
}

/// lowlevel data receiving method.
/** This tries to receive some amount of data over the network. If it does get
 *  nothing (or not enough) within the time it has been accorded, it returns.
 *
 * \param out_pBuf The (allocated) buffer where to write the data.
 * \param in_uiLen The length of the buffer to get.
 *
 * \return If successful:  OK
 * \return If failed:      Error Code
 *
 * \note The user has to allocate the buffer big enough!
 * \note On linux, only an exactitude of 1-10 millisecond may be achieved. Anyway, on most PC's
 *       an exactitude of more the 10ms is nearly never possible.
 * \internal This method is only for internal use!
 *
 * \author Pompei2
 */
FTSC_ERR FTS::TraditionalConnection::get_lowlevel(void *out_pBuf, std::uint32_t in_uiLen)
{
    int read = 0;
    uint32_t to_read = in_uiLen;
    int8_t *buf = (int8_t *)out_pBuf;

    using namespace std::chrono;
    do {
        auto startTime = steady_clock::now();
        read = ::recv( m_sock, (char *) buf, to_read, 0 );
#if WINDOOF
        auto errorno = WSAGetLastError();
        if( read == SOCKET_ERROR && (errorno == WSAEINTR || errorno == WSATRY_AGAIN || errorno == WSAEWOULDBLOCK) ) {
#else
        if( read < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) ) {
#endif
            // Only check for timeouts when waiting for data!
            auto currentTime = steady_clock::now();
            if( duration_cast<milliseconds>(currentTime-startTime).count() > m_maxWaitMillisec ) {
                netlog( "Dropping due to timeout (allowed " + toString( m_maxWaitMillisec ) + " ms)!" );
                return FTSC_ERR::OK;
            }
            continue;
        }

        if(read <= 0) {
            FTS18N("Net_TCPIP_recv", MsgType::Error);
            this->disconnect();
            return FTSC_ERR::RECEIVE;
        }

        to_read -= read;
        buf += read;
    } while(to_read);

    netlog2("recv", this, in_uiLen, (const char *)out_pBuf);

    return FTSC_ERR::OK;
}

/** This tries to receive data over the network until some ending string. If it
 *  does get nothing (or not enough) within the time it has been accorded, it
 *  returns what it got so-far.
 *
 * \param in_sLineEnding The method will stop getting data when it gets this
 *                       exact data-string.
 *
 * \return The data it got.
 *
 * \note On linux, only an exactitude of 1-10 millisecond may be achieved. Anyway, on most PC's
 *       an exactitude of more the 10ms is nearly never possible.
 * \internal This method is only for internal use!
 *
 * \author Pompei2
 */
std::string FTS::TraditionalConnection::getLine(const std::string in_sLineEnding)
{
    std::string sLine;
    uint8_t byte[2] = {0};
    while(this->get_lowlevel(&byte[0], 1) == FTSC_ERR::OK) {
        sLine.push_back( byte[0] );

        // Got an end of line?
        if(sLine.substr(sLine.length() - in_sLineEnding.length(), in_sLineEnding.length()) == in_sLineEnding) {
            return sLine;
        }
    }

    // Connection lost or timed out before EOL.
    return sLine;
}

/// (Waits for and then) receives any packet.
/** This first (by default) looks in the message queue, if there is any message,
 *  it returns that message and removes it from the queue. If the queue is empty
 *  or shall not be used, it waits a certain amount of time, infilitely or not
 *  at all for a message to come over the net.
 *
 * \param in_bUseQueue Use the queue or just ignore it ?
 *
 * \return If successfull: A pointer to the packet.
 * \return If failed:      NULL
 *
 * \note The user has to free the returned value!
 * \note On linux, only an exactitude of 1-10 millisecond may be achieved. Anyway, on most PC's
 *       an exactitude of more the 10ms is nearly never possible.
 * \note This method eats all network incoming data until it finds the FTSS identifier!
 * \internal This method is only for internal use!
 *
 * \author Pompei2
 */
Packet *FTS::TraditionalConnection::getPacket(bool in_bUseQueue)
{
    if(!m_bConnected) {
        FTS18N("InvParam", MsgType::Horror, "FTS::Connection::recv");
        return nullptr;
    }

    // First, check the queue if wanted.
    if(in_bUseQueue) {
        Packet *p = this->getFirstPacketFromQueue();
        if(p)
            return p;
    }

    int serr = 0;

    using namespace std::chrono;
    auto startTime = steady_clock::now();

#if WINDOOF
    fd_set fdr;
    timeval tv = {0, (long) 10000 }; // 10ms

    FD_ZERO( &fdr );
    FD_SET( m_sock, &fdr );

    // Wait an amount of time or wait infinitely
    if(m_maxWaitMillisec == ((uint64_t)(-1)))
        serr = ::select(1, &fdr,NULL, NULL, NULL);
    else
        serr = ::select(1, &fdr,NULL, NULL, &tv);
#else
    do {
        pollfd pfd;
        pfd.fd = m_sock;
        pfd.events = 0 | POLLIN;
        pfd.revents = 0;

        // Wait an amount of time or wait infinitely
        if(m_maxWaitMillisec == ((uint64_t)(-1)))
            serr = ::poll( &pfd, 1, -1 );
        else
            serr = ::poll( &pfd, 1, (int)10000 );
    } while( serr == SOCKET_ERROR && errno == EINTR );
#endif

    if( serr == SOCKET_ERROR ) {
        FTS18N( "Net_TCPIP_select", MsgType::Error, strerror(errno), toString(errno) );
        this->disconnect();
        return nullptr;
    }

    if( serr == 0 ) {
        return nullptr;
    }
    auto currentTime = steady_clock::now();
    if(duration_cast<microseconds>(currentTime-startTime).count() > m_maxWaitMillisec) {
        netlog("Dropping due to timeout (allowed "+toString(m_maxWaitMillisec)+" ms)!");
        return nullptr;
    }

    // First, ignore everything until the "FTSS" identifier.
    int8_t buf;

    while(true) {
        // Get the first "F"
        do {
            if(FTSC_ERR::OK != this->get_lowlevel(&buf, 1)) {
                return nullptr;
            }
        } while(buf != 'F');

        // Check for the next "T". If it isn't one, restart.
        if( FTSC_ERR::OK != this->get_lowlevel( &buf, 1 ) ) {
            return nullptr;
        }

        if(buf != 'T')
            continue;

        // Check for the next "S". If it isn't one, restart.
        if( FTSC_ERR::OK != this->get_lowlevel( &buf, 1 ) ) {
            return nullptr;
        }

        if(buf != 'S')
            continue;

        // Check for the next "S". If it isn't one, restart.
        if( FTSC_ERR::OK != this->get_lowlevel( &buf, 1 ) ) {
            return nullptr;
        }

        if(buf != 'S')
            continue;
        break;
    }

    // We already got the "FTSS" header, now get the rest of the header.
    Packet *p = new Packet(DSRV_MSG_NULL);
    if( FTSC_ERR::OK != this->get_lowlevel( &p->m_pData[4], sizeof( fts_packet_hdr_t ) - 4 ) ) {
        delete p;
        return nullptr;
    }

    // Now, prepare to get the packet's data.
    if( p->getPayloadLen() <= 0 ) {
        FTS18N( "Net_packet_len", MsgType::Error, toString(p->getPayloadLen()) );
        delete p;
        return nullptr;
    }

    p->realloc(p->getTotalLen());
    // And get it.
    if( FTSC_ERR::OK != this->get_lowlevel( p->getPayloadPtr(), p->getPayloadLen() ) ) {
        delete p;
        return nullptr;
    }

    // All is good, check the package ID.
    if(p->isValid()) {
        FTSMSGDBG("Recv packet with ID 0x{1}, payload len: {2}", 5, toString(p->getType(), -1, ' ', std::ios::hex), toString(p->getPayloadLen()));
        addRecvPacketStat(p);
        return p;
    }

    // Invalid packet received.
    FTS18N("Net_packet", MsgType::Error, "No FTSS Header/Invalid request");
    delete p;
    return nullptr;
}

/// Waits for and then receives any packet.
/** This first (by default) looks in the message queue, if there is any message,
 *  it returns that message and removes it from the queue. If the queue is empty
 *  or shall not be used, it waits a certain amount of time for a message to come
 *  over the net. (or waits indefinitely if \a in_nMaxWaitMillisec == 0).
 *
 * \param in_bUseQueue Use the queue or just ignore it ?
 *
 * \return If successfull: A pointer to the packet.
 * \return If failed:      NULL
 *
 * \note The user has to free the returned value !
 * \note On linux, only an exactitude of 1-10 millisecond may be achieved. Anyway, on most PC's
 *       an exactitude of more the 10ms is nearly never possible.
 *
 * \author Pompei2
 */
Packet *FTS::TraditionalConnection::waitForThenGetPacket(bool in_bUseQueue)
{
    return this->getPacket(in_bUseQueue);
}

/// Checks for and then receives any packet.
/** This first (by default) looks in the message queue, if there is any message,
 *  it returns that message and removes it from the queue. If the queue is empty
 *  or shall not be used, it checks if a message is coming over the net. If ther is,
 *  it retrieves it. If there is none, it returns NULL immediately.
 *
 * \param in_bUseQueue Use the queue or just ignore it ?
 *
 * \return If successful: A pointer to the packet.
 * \return If failed:      NULL
 *
 * \note The user has to free the returned value !
 *
 * \author Pompei2
 */
Packet *FTS::TraditionalConnection::getPacketIfPresent(bool in_bUseQueue)
{
    return this->getPacket(in_bUseQueue);
}

/// Waits for and then receives a certain packet.
/** This first looks in the message queue, if there is a certain message,
 *  it returns that message and removes it from the queue. If the queue is empty,
 *  it waits a certain amount of time for a certain message to come over the net.
 *  (or waits indefinitely if \a in_nMaxWaitMillisec == 0).\n
 *  If a message with the wrong request ID comes during this time, it is queued and
 *  waited for the next message.\n
 *  A _certain_ message means a message with a special request ID (as in \a in_req ).
 *
 * \param in_req The request ID of the message to wait for (DSRV_MSG_XXX).
 *
 * \return If successful: A pointer to the packet.
 * \return If failed:      NULL
 *
 * \note The user has to free the returned value !
 * \note On linux, only an exactitude of 1-10 millisecond may be achieved.
 *       Anyway, on most PC's an exactitude of more the 10ms is nearly never possible.
 * \note If the queue is full, the first messages put in it are dropped!
 *
 * \author Pompei2
 */
Packet *FTS::TraditionalConnection::waitForThenGetPacketWithReq(master_request_t in_req)
{
    // Check for valid packet request ID's.
    if(in_req == DSRV_MSG_NONE || in_req > DSRV_MSG_MAX)
        return nullptr;

    // We need to check the queue ourselves to avoid infinite recursion:
    Packet *p = this->getFirstPacketFromQueue(in_req);
    if(p)
        return p;

    // Nothing in the queue, wait for a message.
    do {
        // We don't want recv to handle the queue as we would again add
        // messages to the queue that would cause infinite recursion.
        p = this->waitForThenGetPacket(false);

        // Nothing got in time, bye.
        if(!p)
            return nullptr;

        // Check if this is the packet we want.
        if( p->getType() == in_req) {
            FTSMSGDBG("Accepted packet with ID 0x{1}, payload len: {2}", 5,
                      toString(p->getType(), -1, ' ', std::ios::hex), toString(p->getPayloadLen()));
            return p;
        }

        // If it is not the packet we want, queue this packet.
        this->queuePacket(p);
    } while(true);

    return nullptr;
}

/// Checks for and then receives a certain packet.
/** This first looks in the message queue, if there is a certain message,
 *  it returns that message and removes it from the queue. If the queue is empty,
 *  it checks if a certain message is coming over the net. If there is,
 *  it retrieves it. If there is none, it returns NULL immediately.\n
 *  If there is a message with the wrong request ID, it is queued and tried again.\n
 *  A _certain_ message means a message with a special request ID (as in \a in_req ).
 *
 * \param in_req The request ID of the message to wait for (DSRV_MSG_XXX).
 *
 * \return If successful: A pointer to the packet.
 * \return If failed:      NULL
 *
 * \note The user has to free the returned value !
 * \note If the queue is full, the first messages put in it are dropped!
 *
 * \author Pompei2
 */
Packet *FTS::TraditionalConnection::getPacketWithReqIfPresent(master_request_t in_req)
{
    // Check for valid packet request ID's.
    if(in_req == DSRV_MSG_NONE || in_req > DSRV_MSG_MAX)
        return nullptr;

    // We need to check the queue ourselves to avoid infinite recursion:
    Packet *p = this->getFirstPacketFromQueue(in_req);
    if(p)
        return p;

    // Nothing in the queue, check for a message.
    while( (p = this->getPacketIfPresent( true )) != nullptr ) {

        // Check if this is the packet we want.
        if( p->getType() == in_req) {
            FTSMSGDBG( "Accepted packet with ID 0x{1}, payload len: {2}", 5,
                      toString(p->getType(), -1, ' ', std::ios::hex), toString(p->getPayloadLen()));
            return p;
        }

        // If it is not the packet we want, queue this packet and try again
        this->queuePacket(p);
    }

    // No packet on the wire.
    return nullptr;
}

/// Sends some data.
/** This sends some data to the pc this connection is with.
 *
 * \param in_pData A pointer to the data to send.
 * \param in_uiLen The length of the data to send.
 *
 * \return If successful: OK
 * \return If failed:     Error code
 *
 * \todo Add a select here too. ?????
 *
 * \author Pompei2
 */
FTSC_ERR FTS::TraditionalConnection::send( const void *in_pData, std::uint32_t in_uiLen )
{
    if(!m_bConnected)
        return FTSC_ERR::NOT_CONNECTED;

    errno = 0;

    int iSent = 0;
    uint32_t uiToSend = in_uiLen;
    const int8_t *buf = (const int8_t *)in_pData;

    do {
#if WINDOOF
        iSent = ::send(m_sock, (const char *)buf, uiToSend, 0);
        if(iSent == SOCKET_ERROR && (WSAGetLastError() == WSAEINTR ||
                                     WSAGetLastError() == WSATRY_AGAIN ||
                                     WSAGetLastError() == WSAEWOULDBLOCK))
#else
        iSent = ::send(m_sock, buf, uiToSend, 0);
        if(iSent < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
#endif
            continue;
        if(iSent < 0) {
            FTS18N("Net_TCPIP_send", MsgType::Error, strerror(errno), toString(errno));
            return FTSC_ERR::SEND;
        }
        uiToSend -= iSent;
        buf += iSent;
    } while(uiToSend > 0);

    netlog2("send", this, in_uiLen, (const char *)in_pData);

    return FTSC_ERR::OK;
}

/// Sends a packet.
/** This sends a packet to the pc this connection is with.
 *
 * \param in_pPacket A pointer to the packet to send.
 *
 * \return If successful: OK
 * \return If failed:     Error code 
 *
 * \author Pompei2
 */
FTSC_ERR FTS::TraditionalConnection::send( Packet * in_pPacket )
{
    if( !m_bConnected )
        return FTSC_ERR::NOT_CONNECTED;

    if( in_pPacket == nullptr )
        return FTSC_ERR::INVALID_INPUT;

    FTSMSGDBG("Sending packet with ID 0x{1}, payload len: {2}", 5, toString(in_pPacket->getType(), -1, ' ', std::ios::hex), toString(in_pPacket->getPayloadLen()));
    addSendPacketStat(in_pPacket);

    return this->send( in_pPacket->m_pData, in_pPacket->getTotalLen() );
}

/*! The request packet is send to the master server. The function waits until the whole
 * response is received or time out is elapsed. The response is checked for the
 * right ID in the header.\n
 * The input packet is destroyed and replaced by the response packet.
 *
 * @author Klaus.Beyer
 *
 * @param[out] out_pPacket The packet to send. Will be replaced by the response.
 *
 * @return If successful: OK
 * @return If failed:     Error code 
 *
 * @note Adapted by Pompei2.
 */
FTSC_ERR FTS::TraditionalConnection::mreq(Packet *out_pPacket)
{
    if(!m_bConnected) {
        return FTSC_ERR::NOT_CONNECTED;
    }

    master_request_t req = out_pPacket->getType();
    if(req == DSRV_MSG_NULL || req == DSRV_MSG_NONE || req > DSRV_MSG_MAX ) {
        return FTSC_ERR::WRONG_REQ;
    }

    if( this->send( out_pPacket ) != FTSC_ERR::OK ) {
        FTS18N( "Net_TCPIP_send", MsgType::Error, strerror( errno ), toString(errno) );
        return FTSC_ERR::SEND;
    }

    Packet *p = this->waitForThenGetPacketWithReq(req);
    if( p == nullptr ) {
        return FTSC_ERR::RECEIVE;
    }

    if(p->getType() != req) {
        master_request_t id = p->getType();
        FTS18N("Net_packet", MsgType::Error, "got id "+toString(id)+", wanted "+toString(req));
        delete p;
        return FTSC_ERR::WRONG_RSP;
    }

    // Transfer the receive buffer to the in packet
    out_pPacket->transferData( p );
    
    // delete the interim packet
    delete p;

    out_pPacket->rewind();
    return FTSC_ERR::OK;
}

/*! Change a sockets blocking mode.
 *
 * @param[in] in_socket      socket to change the blocking mode
 * @param[in] in_bBlocking   true: enable blocking; false: disable blocking
 *
 * @return 0 on success ; -1 on error.
 *
 * @note
 *
 */
int FTS::TraditionalConnection::setSocketBlocking(SOCKET in_socket, bool in_bBlocking)
{
#if WINDOOF
    u_long ulMode = in_bBlocking ? 0 : 1;

    return ioctlsocket(in_socket, FIONBIO, &ulMode);
#else
    int flags;

    if(in_bBlocking) {
        if((flags = fcntl(in_socket, F_GETFL, 0)) < 0) {
            FTS18N("Net_TCPIP_fcntl_get", MsgType::Error, strerror(errno), toString(errno));
            return -1;
        }

        if(fcntl(in_socket, F_SETFL, flags & (~O_NONBLOCK)) < 0) {
            FTS18N("Net_TCPIP_fcntl_set", MsgType::Error, strerror(errno), toString(errno));
            return -1;
        }
    } else {
        if((flags = fcntl(in_socket, F_GETFL, 0)) < 0) {
            FTS18N("Net_TCPIP_fcntl_get", MsgType::Error, strerror(errno), toString(errno));
            return -1;
        }

        if(fcntl(in_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
            FTS18N("Net_TCPIP_fcntl_set", MsgType::Error, strerror(errno), toString(errno));
            return -1;
        }
    }
    return 0;
#endif
}

/// Gets a file via HTTP.
/** This sends an HTTP server the request to get a file and then gets that file
 *  from the server.
 *
 * \param out_data Where to store the data of the file.
 * \param in_sServer The server address to connect to, ex: arkana-fts.org
 * \param in_sPath The path to the file on the server, ex: /path/to/file.ex
 * \param out_uiFileSize Will be set to the size of the data that will be returned.
 *
 * \return If successfull:  ERR_OK=0.
 * \return If failed:       An error code < 0
 *
 * \note The DataContainer object you give will be resized to match the file's
 *       size and the content will then be overwritten.
 * \note On linux, only an exactitude of 1-10 millisecond may be achieved. Anyway, on most PC's
 *       an exactitude of more the 10ms is nearly never possible.
 * \note Although the \a out_uiFileSize is a 64-bit unsigned integer, the
 *       support for large files (>4GB) is not implemented yet.
 *
 * \author Pompei2
 */
int FTS::getHTTPFile(FTS::RawDataContainer &out_data, const std::string &in_sServer, const std::string &in_sPath, std::uint64_t in_ulMaxWaitMillisec)
{
    // We connect using the traditional connection.
    TraditionalConnection tradConn(in_sServer, 80, in_ulMaxWaitMillisec);
    if(!tradConn.isConnected())
        return -1;

    // Send the request to get that file.
    std::string sToSend = "GET http://" + in_sServer + in_sPath + " HTTP/1.0\r\n\r\n";
    tradConn.send(sToSend.c_str(), sToSend.length());

    // The first line we get is very interesting: the status.
    std::string sLine = tradConn.getLine("\r\n");
    std::string sCode = sLine.substr(9,sLine.length()-12);
    if(sCode != "200") {
        // 200 means OK.
        return -2;
    }

    // The following loop reads out the HTTP header and gets the data size.
    uint64_t uiFileSize = 0;
    while(true) {
        std::string sHttpLine = tradConn.getLine("\r\n");

        // Reached the end.
        if(sHttpLine.empty() || sHttpLine == "\r\n") {
            break;
        }

        // This line is interesting.
        if(sHttpLine.substr(0,16) == "Content-Length: ") {
            long long int i = 0;
            if( sscanf(sHttpLine.c_str(), "Content-Length: %lld", &i) == 1 ) {
                uiFileSize = static_cast<uint64_t>(i);
            }
        }
    }

    // Did not get any header about the data length.
    if(uiFileSize == 0)
        return -3;

    // Get the memory to write the data in.
    out_data.resize(uiFileSize);

    // Get the data.
    if( FTSC_ERR::OK != tradConn.get_lowlevel(out_data.getData(), static_cast<uint32_t>(uiFileSize)) ) {
        return -4;
    }

    return 0;
}

/// Gets a file via HTTP.
/** This sends an HTTP server the request to get a file and then gets that file
 *  from the server.
 *
 * \param in_sServer The server address to connect to, ex: arkana-fts.org
 * \param in_sPath The path to the file on the server, ex: /path/to/file.ex
 * \param out_uiFileSize Will be set to the size of the data that will be returned.
 *
 * \return If successful:  A new DataContainer containing the file.
 * \return If failed:       NULL
 *
 * \note The DataContainer object you get should be deleted by you.
 * \note On linux, only an exactitude of 1-10 millisecond may be achieved. Anyway, on most PC's
 *       an exactitude of more the 10ms is nearly never possible.
 * \note Although the \a out_uiFileSize is a 64-bit unsigned integer, the
 *       support for large files (>4GB) is not implemented yet.
 *
 * \author Pompei2
 */
FTS::RawDataContainer *FTS::getHTTPFile(const std::string &in_sServer, const std::string &in_sPath, std::uint64_t in_ulMaxWaitMillisec)
{
    // Get the memory to write the data in.
    FTS::RawDataContainer *pdc = new FTS::RawDataContainer(0);

    if( 0 != FTS::getHTTPFile( *pdc, in_sServer, in_sPath, in_ulMaxWaitMillisec ) ) {
        delete pdc;
        pdc = nullptr;
    }
    return pdc;
}

/// Downloads a file via HTTP and saves it locally.
/** This sends an HTTP server the request to get a file and then gets that file
 *  from the server. Then it stores the file locally.
 *
 * \param in_sServer The server address to connect to, ex: arkana-fts.org
 * \param in_sPath The path to the file on the server, ex: /path/to/file.ex
 * \param in_sLocal The path to the file to create/overwrite to save the data to
 *                  . This path is on the local machine.
 * \param in_ulMaxWaitMillisec The amount of milliseconds (1/1000 seconds) to
 *                             wait for a message to come.
 *
 * \return If successful:  ERR_OK
 * \return If failed:       An error code < 0
 *
 * \note If the file pointed to by \a in_sLocal already exists, it will be overwritten.
 * \note On linux, only an exactitude of 1-10 millisecond may be achieved. Anyway, on most PC's
 *       an exactitude of more the 10ms is nearly never possible.
 *
 * \author Pompei2
 */
int FTS::downloadHTTPFile(const std::string &in_sServer, const std::string &in_sPath, const std::string &in_sLocal, std::uint64_t in_ulMaxWaitMillisec)
{
    FTS::DataContainer *pData = FTS::getHTTPFile(in_sServer, in_sPath, in_ulMaxWaitMillisec);
    if(pData == nullptr)
        return -1;

    FILE *pFile = fopen(in_sLocal.c_str(), "w+b");
    if(!pFile) {
        FTS18N("File_FopenW", MsgType::Error, in_sLocal, strerror(errno));
        delete pData;
        return -2;
    }

    bool bSuccess = fwrite(pData->getData(), static_cast<size_t>(pData->getSize()), 1, pFile) == 1;
    if(!bSuccess)
        FTS18N("File_Write", MsgType::Error, in_sLocal, strerror(errno));

    delete pData;
    fclose(pFile);

    return bSuccess ? 0 : -3;
}
