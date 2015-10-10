/**
 * \file connection.h
 * \author Pompei2
 * \date 11 May 2007
 * \brief This file describes the class that represents a network
 *        connection that can send packets.
 **/

#ifndef FTS_CONNECTION_H
#define FTS_CONNECTION_H

#if defined( _WIN32 )
#  include <Winsock2.h>
#  define WINDOOF 1
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netinet/ip.h>
   using SOCKET = int;

#endif

using SOCKADDR_IN = sockaddr_in;

#include <list>
#include <sstream>

#ifndef SOCKET_ERROR
#  define SOCKET_ERROR -1
#endif

#include "packet.h"

#define FTSC_TIME_OUT      1000    ///< time out value in milliseconds
#define FTSC_MAX_QUEUE_LEN 32      ///< The longest queue we shall have. If queue gets longer, drop it.

template <class T>
static inline std::string toString( const T& t, std::streamsize in_iWidth = 0, char in_cFill = ' ', std::ios_base::fmtflags in_fmtfl = std::ios::dec, typename std::enable_if< std::is_integral<T>::value >::type* = 0 )
{
    std::stringstream out;
    out.width( in_iWidth );
    out.fill( in_cFill );
    out.flags( in_fmtfl );

    out << t;
    return out.str();
}


enum class FTSC_ERR {
    OK            =  0, ///< No error.
    NOT_CONNECTED = -1, ///< The connection is not connected.
    SEND          = -2, ///< Socket error on send()
    SELECT        = -3, ///< Socket error on select
    TIMEOUT       = -4, ///< select() w/ has timed out
    RECEIVE       = -5, ///< Socket error on recv
    WRONG_RSP     = -6, ///< Response doesn't match the request
    WRONG_REQ     = -7, ///< Invalid request
    HOST_NAME     = -8, ///< Get Host by name failed.
    SOCKET        = -9, ///< Socket error.
    INVALID_INPUT = -10, ///< Invalid method parameter. Usually a nullptr.
};

namespace FTS {
    class RawDataContainer;

/// The FTS connection class
/** This class represents an abstract connection.
 *  It may be implemented as a connection over tcp/ip, over serial,
 *  over pipes or whatever you want.
 **/
class Connection {
public:
    virtual ~Connection() {};

    enum class eConnectionType
    {
        D_CONNECTION_TRADITIONAL  = 0x0,
        D_CONNECTION_ONDEMAND_CLI = 0x1,
        D_CONNECTION_ONDEMAND_SRV = 0x2
    } ;

    virtual eConnectionType getType() const = 0;

    virtual bool isConnected() = 0;
    virtual void disconnect() = 0;

    virtual std::string getCounterpartIP() const = 0;

    virtual Packet *waitForThenGetPacket(bool in_bUseQueue = true) = 0;
    virtual Packet *getPacketIfPresent(bool in_bUseQueue = true) = 0;
    virtual Packet *getReceivedPacketIfAny() = 0 ;
    virtual FTSC_ERR send(Packet *in_pPacket) = 0;
    virtual FTSC_ERR mreq(Packet *in_pPacket) = 0;

    virtual void setMaxWaitMillisec( std::uint64_t in_ulMaxWaitMillisec ) { m_maxWaitMillisec = in_ulMaxWaitMillisec; }
protected:
    std::list<Packet *>m_lpPacketQueue; ///< A queue of packets that have been received but not consumed. Most recent are at the back.
    std::uint64_t m_maxWaitMillisec;         ///< Time out in millisec for all socket calls.
    
    Connection() : m_maxWaitMillisec( FTSC_TIME_OUT ) {};
    virtual Packet *getFirstPacketFromQueue(master_request_t in_req = DSRV_MSG_NONE);
    virtual void queuePacket(Packet *in_pPacket);
};

int getHTTPFile(FTS::RawDataContainer &out_data, const std::string &in_sServer, const std::string &in_sPath, std::uint64_t in_ulMaxWaitMillisec);
FTS::RawDataContainer *getHTTPFile(const std::string &in_sServer, const std::string &in_sPath, std::uint64_t in_ulMaxWaitMillisec);
int downloadHTTPFile(const std::string &in_sServer, const std::string &in_sPath, const std::string &in_sLocal, std::uint64_t in_ulMaxWaitMillisec);

/// A Traditional TCP/IP implementation of the connection class.
/**
 * This class is an implementation of the Connection class and implements
 * a network connection over TCP/IP using sockets.\n
 * \n
 * The connection is done one time
 * and then never done again. At the end, the connection is closed.\n
 * \n
 * Read more details
 * about it in our DokuWiki design documents->networking section, direct link:
 * http://wiki.arkana-fts.org/doku.php?id=design_documents:networking:src:connection_types
 **/
class TraditionalConnection : public Connection {
    friend class OnDemandHTTPConnection;
    friend int FTS::getHTTPFile(FTS::RawDataContainer &out_data, const std::string &in_sServer, const std::string &in_sPath, std::uint64_t in_ulMaxWaitMillisec);

public:
    TraditionalConnection(const std::string &in_sName, std::uint16_t in_usPort, std::uint64_t in_ulTimeoutInMillisec);
    TraditionalConnection(SOCKET in_sock, SOCKADDR_IN in_sa);
    virtual ~TraditionalConnection();

    eConnectionType getType() const { return eConnectionType::D_CONNECTION_TRADITIONAL; }

    virtual bool isConnected();
    virtual void disconnect();

    virtual std::string getCounterpartIP() const;

    virtual Packet *waitForThenGetPacket(bool in_bUseQueue = true);
    virtual Packet *getPacketIfPresent(bool in_bUseQueue = true);
    virtual Packet *getReceivedPacketIfAny() ;

    virtual Packet *waitForThenGetPacketWithReq(master_request_t in_req);
    virtual Packet *getPacketWithReqIfPresent(master_request_t in_req);

    virtual FTSC_ERR send( Packet *in_pPacket );
    virtual FTSC_ERR mreq(Packet *in_pPacket);

    static int setSocketBlocking(SOCKET out_socket, bool in_bBlocking);

protected:
    bool m_bConnected;              ///< Wether the connection is up or not.
    SOCKET m_sock;                  ///< The connection socket.
    SOCKADDR_IN m_saCounterpart;    ///< This is the address of our counterpart.

    FTSC_ERR connectByName( std::string in_sName, std::uint16_t in_usPort);
    virtual Packet *getPacket(bool in_bUseQueue);
    virtual FTSC_ERR get_lowlevel(void *out_pBuf, std::uint32_t in_uiLen);
    virtual std::string getLine(const std::string in_sLineEnding);

    virtual FTSC_ERR send( const void *in_pData, std::uint32_t in_uiLen );
};

}

#endif /* FTS_CONNECTION_H */

 /* EOF */
