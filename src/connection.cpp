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
#include "Logger.h"
#include "TraditionalConnection.h"


using namespace FTS;

Connection * FTS::Connection::create( eConnectionType type, const std::string &in_sName, std::uint16_t in_usPort, std::uint64_t in_ulTimeoutInMillisec )
{
    switch( type ) {
        case eConnectionType::D_CONNECTION_TRADITIONAL:
            return new TraditionalConnection( in_sName, in_usPort, in_ulTimeoutInMillisec );
        default:
            return nullptr;
    }

    return nullptr;
}

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

    if( p != nullptr && Logger::DbgLevel() == 5) {
        FTSMSGDBG("Recv packet from queue with ID 0x{1}, payload len: {2}", 4, toString(p->getType(), -1, ' ', std::ios::hex), toString(p->getPayloadLen()));
        std::string s = "Queue is now: (len:"+toString(m_lpPacketQueue.size())+")";
        for(auto pPack : m_lpPacketQueue) {
            s += "(0x" + toString(pPack->getType(), -1, ' ', std::ios::hex) + "," + toString(pPack->getPayloadLen()) + ")";
        }
        s += "End.";
        FTSMSGDBG(s, 4);
    }

    return p;
}

/// Adds a packet to the queue.
/** This first adds a packet at the end of the queue, then checks if the queue is
 *  too big. If it is, it drops out the first packet in the front of the queue.
 *  Also displays debugging messages about what it does.
 *
 * \author Pompei2
 */
void FTS::Connection::queuePacket( Packet *in_pPacket )
{
    if( !in_pPacket )
        return;

    m_lpPacketQueue.push_back( in_pPacket );

    // Don't make the queue too big.
    while( m_lpPacketQueue.size() > FTSC_MAX_QUEUE_LEN ) {
        Packet *pPack = m_lpPacketQueue.front();
        FTSMSGDBG( "Queue full, dropping packet with ID 0x{1}, payload len: {2}", 5,
                   toString( pPack->getType(), -1, ' ', std::ios::hex ), toString( pPack->getPayloadLen() ) );
        m_lpPacketQueue.pop_front();
        delete pPack;
    }

    if( Logger::DbgLevel() == 5 ) {

        FTSMSGDBG( "Queued packet with ID 0x{1}, payload len: {2}", 5,
                   toString( in_pPacket->getType(), -1, ' ', std::ios::hex ), toString( in_pPacket->getPayloadLen() ) );
        std::string s = "Queue is now: (len:" + toString( m_lpPacketQueue.size() ) + ")";
        for( auto pPack : m_lpPacketQueue ) {
            s += "(0x" + toString( pPack->getType(), -1, ' ', std::ios::hex ) + "," + toString( pPack->getPayloadLen() ) + ")";
        }
        s += "End.";
        FTSMSGDBG( s, 5 );
    }
}

void FTS::Connection::addSendPacketStat( Packet * p )
{
    ++m_statPackets[p->getType()].second;
}

void FTS::Connection::addRecvPacketStat( Packet * p )
{
    ++m_statPackets[p->getType()].first;
}

