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

namespace FTS {

/// A Traditional TCP/IP implementation of the connection class.
/**
 * This class is an implementation of the Connection class and implements
 * a network connection over TCP/IP using sockets.\n
 * \n
 * The connection is done one time
 * and then never done again. At the end, the connection is closed.\n
 * \n
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
    virtual Packet *getReceivedPacketIfAny() ;

    virtual Packet *waitForThenGetPacketWithReq(master_request_t in_req);

    virtual FTSC_ERR send( Packet *in_pPacket );
    virtual FTSC_ERR mreq(Packet *in_pPacket);
    static int setSocketBlocking( SOCKET in_socket, bool in_bBlocking );

protected:
    bool m_bConnected;              ///< Wether the connection is up or not.
    SOCKET m_sock;                  ///< The connection socket.
    SOCKADDR_IN m_saCounterpart;    ///< This is the address of our counterpart.

    FTSC_ERR connectByName( std::string in_sName, std::uint16_t in_usPort);
    virtual Packet *getPacket(bool in_bUseQueue, uint64_t timeOut = 0);
    virtual FTSC_ERR get_lowlevel(void *out_pBuf, std::size_t in_uiLen);
    virtual std::string getLine(const std::string& in_sLineEnding);

    virtual FTSC_ERR send( const void *in_pData, std::size_t in_uiLen );

private:
    void netlog( const std::string &in_s ); 
    void netlog2( const std::string &in_s, const void* id, size_t in_uiLen, const char *in_pBuf );

};

}

#endif /* FTS_TRADITIONALCONNECTION_H */

 /* EOF */
