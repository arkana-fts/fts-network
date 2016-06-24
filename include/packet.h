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

public:
    Packet( const Packet &in_copy ) = delete ; ///< Block the copy-constructor.
    Packet() = delete;                         ///< Block the default-constructor.
    Packet( Packet&& in_packet ) noexcept;
    Packet& operator=( Packet&& in_packet ) noexcept;
    Packet& operator=(const Packet& in_packet) = delete; ///< Block the assigment operator

    Packet(master_request_t in_cType);
    virtual ~Packet();

    bool isValid() const;

    Packet *setType(master_request_t in_cType);
    master_request_t getType() const;

    std::size_t getTotalLen() const;
    std::size_t getPayloadLen() const;
    Packet *rewind();
    Packet *realloc( std::size_t in_newSize );
    std::int8_t* getPayloadPtr() const { return getDataPtr(); }
    Packet* transferData( Packet* p );

    Packet *append(std::string in);
    Packet *append(const void *in_pData, std::size_t in_iSize);

    /// Appends something to the message.
    /** This appends something at the current cursor position in the message.
    *  After adding the data, the cursor is moved to point right behind it.
    *
    * \param in The data to append.
    *
    * \return a pointer to itself (this)
    *
    * \note If there is not enough space to hold it, nothing is stored.
    *
    * \author Pompei2
    */
    template<class T, typename std::enable_if<std::is_integral<T>::value || std::is_enum<T>::value>::type* = nullptr>
    Packet* append( T in )
    {
        std::int8_t* pBuf;
        pBuf = (std::int8_t *)::realloc( (void *) m_pData, sizeof( T ) + m_uiCursor );
        if( pBuf != nullptr ) {
            m_pData = pBuf;
            *((T *) & m_pData[m_uiCursor]) = in;
            m_uiCursor += sizeof( T );
            ((fts_packet_hdr_t*) m_pData)->data_len = (std::uint32_t) (m_uiCursor - D_PACKET_HDR_LEN);
        }
        return this;
    }

    /// Retrieves something from the message.
    /** This retrieves something from the current cursor position in the message.
    *  After retrieving the data, the cursor is moved to point right behind it.
    *
    * \param in Reference to the data to retrieve. Ff there was an error, this is set to 0.
    *
    * \return nothing.
    *
    * \author Pompei2
    */
    template<class T, typename std::enable_if<std::is_integral<T>::value || std::is_enum<T>::value>::type* = nullptr >
    void get( T& in )
    {
        std::size_t len = this->getTotalLen();
        if( (m_uiCursor >= len) || ( (m_uiCursor + sizeof( T )) > len) ) {
            in = (T) 0;
            return;
        }

        in = *((T *) (&m_pData[m_uiCursor]));
        m_uiCursor += sizeof( T );
    }

    inline std::int8_t get() { std::int8_t out = 0; this->get(out); return out;}
    inline void get(std::string& out) {out = this->get_string(); }
    std::string get_string();
    std::string extractString();
    int get(void *out_pData, std::size_t in_iSize);

    int writeToPacket(Packet *in_pPack);
    int readFromPacket(Packet *in_pPack);

    int printToFile(FILE *in_pFile) const;

private:
    std::int8_t *m_pData;     ///< The data this packet contains.
    std::size_t m_uiCursor;   ///< The current cursor position in the data.

    /// Returns a pointer to the beginning of the data.
    inline std::int8_t *getDataPtr() const            {return &m_pData[D_PACKET_HDR_LEN];}
    inline const std::int8_t *getConstDataPtr() const {return &m_pData[D_PACKET_HDR_LEN];}
};

}

#endif /* FTS_PACKET_H */

 /* EOF */
