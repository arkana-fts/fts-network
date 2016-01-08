/**
 * \file connection.h
 * \author Pompei2
 * \date 11 May 2007
 * \brief This file describes the class that represents a network
 *        connection that can send packets.
 **/

#ifndef FTS_CONNECTION_H
#define FTS_CONNECTION_H

#include <vector>
#include <list>
#include <unordered_map>
#include <cstdint>

#include "packet.h"

#define FTSC_TIME_OUT      1000    ///< time out value in milliseconds
#define FTSC_MAX_QUEUE_LEN 32      ///< The longest queue we shall have. If queue gets longer, drop it.

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

// Holds for each request the recv and send counts.
using PacketStats = std::unordered_map<master_request_t, std::pair<std::uint64_t, std::uint64_t>>;

namespace FTS {

FTSC_ERR getHTTPFile(std::vector<std::uint8_t>& out_data, const std::string &in_sServer, const std::string &in_sPath, std::uint64_t in_ulMaxWaitMillisec );
int downloadHTTPFile( const std::string &in_sServer, const std::string &in_sPath, const std::string &in_sLocal, std::uint64_t in_ulMaxWaitMillisec );

/// The FTS connection class
/** This class represents an abstract connection.
 *  It may be implemented as a connection over tcp/ip, over serial,
 *  over pipes or whatever you want.
 **/
class Connection 
{
public:
    virtual ~Connection() {};

    enum class eConnectionType
    {
        D_CONNECTION_TRADITIONAL  = 0x0,
        D_CONNECTION_ONDEMAND_CLI = 0x1,
        D_CONNECTION_ONDEMAND_SRV = 0x2
    } ;

    static Connection* create( eConnectionType type, const std::string &in_sName, std::uint16_t in_usPort, std::uint64_t in_ulTimeoutInMillisec );
    virtual eConnectionType getType() const = 0;
    virtual bool isConnected() = 0;
    virtual void disconnect() = 0;

    virtual std::string getCounterpartIP() const = 0;

    virtual Packet *waitForThenGetPacket(bool in_bUseQueue = true) = 0;
    virtual Packet *getReceivedPacketIfAny() = 0 ;
    virtual FTSC_ERR send(Packet *in_pPacket) = 0;
    virtual FTSC_ERR mreq(Packet *in_pPacket) = 0;

    virtual void setMaxWaitMillisec( std::uint64_t in_ulMaxWaitMillisec ) { m_maxWaitMillisec = in_ulMaxWaitMillisec; }
    PacketStats getPacketStats() { return m_statPackets; }
protected:
    std::list<Packet *>m_lpPacketQueue; ///< A queue of packets that have been received but not consumed. Most recent are at the back.
    std::uint64_t m_maxWaitMillisec;         ///< Time out in millisec for all socket calls.

    Connection() : m_maxWaitMillisec( FTSC_TIME_OUT ) {};
    virtual Packet *getFirstPacketFromQueue(master_request_t in_req = DSRV_MSG_NONE);
    virtual void queuePacket(Packet *in_pPacket);
    // Statistical information
    void addSendPacketStat( Packet* p );
    void addRecvPacketStat( Packet* p );
private:
    PacketStats m_statPackets;
};

}

#endif /* FTS_CONNECTION_H */

 /* EOF */
