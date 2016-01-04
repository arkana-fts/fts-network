/**
 * \file TraditionalConnection.h
 * \author Pompei2
 * \date 11 May 2007
 * \brief This file describes the class that represents a network
 *        connection that can send packets.
 **/

#ifndef FTS_TRADITIONALCONNECTION_H
#define FTS_TRADITIONALCONNECTION_H

#if defined( _WIN32 )
#  include <Winsock2.h>
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netinet/ip.h>
   using SOCKET = int;

#endif

using SOCKADDR_IN = struct sockaddr_in;

#include <list>
#include <sstream>
#include <algorithm>
#include <unordered_map>

#ifndef SOCKET_ERROR
#  define SOCKET_ERROR -1
#endif

#include "connection.h"
#include "packet.h"

#define FTSC_TIME_OUT      1000    ///< time out value in milliseconds
#define FTSC_MAX_QUEUE_LEN 32      ///< The longest queue we shall have. If queue gets longer, drop it.


namespace FTS {

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
    friend FTSC_ERR getHTTPFile( std::vector<uint8_t>& out_data, const std::string &in_sServer, const std::string &in_sPath, std::uint64_t in_ulMaxWaitMillisec );

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
    static int setSocketBlocking( SOCKET in_socket, bool in_bBlocking );

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

#endif /* FTS_TRADITIONALCONNECTION_H */

 /* EOF */
