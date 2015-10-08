/**
 * \file packet.h
 * \author Pompei2
 * \date 11 May 2007
 * \brief This file describes the class that represents a packet
 *        that gets sent over a connection.
 **/

#ifndef FTS_PACKET_H
#define FTS_PACKET_H

#include <string>

#include "packet_header.h"

namespace FTS {

/// The FTS packet class
/** This class represents a packet that can be sent over a connection.
 *  This connection is described by another class.
 **/
class Packet {
    friend class Connection;
    friend class TraditionalConnection;
    friend class OnDemandHTTPConnection;

private:
    int8_t *m_pData;     ///< The data this packet contains.
    uint32_t m_uiCursor; ///< The current cursor position in the data.


public:
    Packet( const Packet &in_copy ) = delete ; ///< Block the copy-constructor.
    Packet() = delete; ///< Block the default-constructor.
    Packet(master_request_t in_cType);
    virtual ~Packet();

    bool isValid() const;

    Packet *setType(master_request_t in_cType);
    master_request_t getType() const;

    uint32_t getTotalLen() const;
    uint32_t getPayloadLen()const;
    Packet *rewind();
    Packet *realloc( size_t in_newSize );
    uint8_t* getPayloadPtr() const { return ( uint8_t* ) &m_pData[sizeof( fts_packet_hdr_t )]; }
    Packet* transferData( Packet* p );


    Packet *append(const std::string & in);
    Packet *append(const void *in_pData, uint32_t in_iSize);

    /// Appends something to the message.
    /** This appends something at the current cursor position in the message.
    *  After adding the data, the cursor is moved to point right behind it.
    *
    * \param in The data to append.
    *
    * \return a pointer to itself (this)
    *
    * \note the data can be anything, like an int, float, .. even a struct.
    *       Of course, pointers gets only their address stored.
    *       If there is not enough space to hold it, nothing is stored.
    *
    * \author Pompei2
    */
    template<class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    Packet* append( T in )
    {
        int8_t* pBuf;
        pBuf = (int8_t *)::realloc( (void *) m_pData, sizeof( T ) + m_uiCursor );
        if( pBuf != nullptr ) {
            m_pData = pBuf;
            *((T *) & m_pData[m_uiCursor]) = in;
            m_uiCursor += sizeof( T );
            ((fts_packet_hdr_t*) m_pData)->data_len = m_uiCursor - D_PACKET_HDR_LEN;
        }
        return this;
    }

    /// Retrieves something from the message.
    /** This retrieves something from the current cursor position in the message.
    *  After retrieving the data, the cursor is moved to point right behind it.
    *
    * \param in Reference to the data to retrieve. if there was an error, this is set to 0.
    *
    * \return nothing.
    *
    * \note the data can be anything, like an int, float, .. even a struct.
    *       THIS FUNCTION IS DANGEROUS as it does not do any bound-checking,
    *       what means that you can get more then there is !
    *
    *       Added a try to bound-check. TODO: Test it.
    *
    * \author Pompei2
    */
    template<class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr > 
    void get( T& in )
    {
        size_t len = this->getTotalLen();
        if( m_uiCursor >= len || m_uiCursor + sizeof( T ) > len ) {
            in = (T) 0;
            return;
        }

        in = *((T *) (&m_pData[m_uiCursor]));
        m_uiCursor += sizeof( T );
        return;
    }

    inline int8_t get() {int8_t out = 0; this->get(out); return out;};
    inline std::string get( std::string &out) {out = this->get_string(); return out;};
    std::string get_string();
    std::string extractString();
    int get(void *out_pData, uint32_t in_iSize);

    int writeToPacket(Packet *in_pPack);
    int readFromPacket(Packet *in_pPack);

    int printToFile(FILE *in_pFile) const;

private:

    /// Returns a pointer to the beginning of the data.
    inline int8_t *getDataPtr() {return &m_pData[D_PACKET_HDR_LEN];};
    inline const int8_t *getConstDataPtr() const {return &m_pData[D_PACKET_HDR_LEN];};
};

}

#endif /* FTS_PACKET_H */

 /* EOF */
