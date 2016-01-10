#ifndef FTS_PACKETHEADER_H
#define FTS_PACKETHEADER_H

using master_request_t = std::uint8_t;

#pragma pack(push, 1)
#pragma pack(1)
struct fts_packet_hdr_t
{
    std::int8_t ident[4];           ///< 'FTSS'
    master_request_t req_id;        ///< DSRV_MSG_ constant
    std::uint32_t data_len;         ///< following size of data in bytes
} ;
#pragma pack(pop)

constexpr size_t D_PACKET_HDR_LEN = sizeof( fts_packet_hdr_t );

// The message types.
/////////////////////
#  define DSRV_MSG_NULL               ((master_request_t)0x00)
#  define DSRV_MSG_LOGIN              ((master_request_t)0x01)
#  define DSRV_MSG_LOGOUT             ((master_request_t)0x02)
#  define DSRV_MSG_SIGNUP             ((master_request_t)0x03)
#  define DSRV_MSG_FEEDBACK           ((master_request_t)0x04)
#  define DSRV_MSG_PLAYER_SET         ((master_request_t)0x10)
#  define DSRV_MSG_PLAYER_GET         ((master_request_t)0x11)
#  define DSRV_MSG_PLAYER_SET_FLAG    ((master_request_t)0x12)
#  define DSRV_MSG_GAME_INS           ((master_request_t)0x20)
#  define DSRV_MSG_GAME_REM           ((master_request_t)0x21)
#  define DSRV_MSG_GAME_LST           ((master_request_t)0x22)
#  define DSRV_MSG_GAME_INFO          ((master_request_t)0x23)
#  define DSRV_MSG_GAME_START         ((master_request_t)0x24)
#  define DSRV_MSG_CHAT_SENDMSG       ((master_request_t)0x30)
#  define DSRV_MSG_CHAT_GETMSG        ((master_request_t)0x31)
#  define DSRV_MSG_CHAT_IUNAI         ((master_request_t)0x32)
#  define DSRV_MSG_CHAT_JOIN          ((master_request_t)0x33)
#  define DSRV_MSG_CHAT_JOINS         ((master_request_t)0x34)
#  define DSRV_MSG_CHAT_QUITS         ((master_request_t)0x35)
#  define DSRV_MSG_CHAT_MOTTO_GET     ((master_request_t)0x36)
#  define DSRV_MSG_CHAT_MOTTO_SET     ((master_request_t)0x37)
#  define DSRV_MSG_CHAT_MOTTO_CHANGED ((master_request_t)0x38)
#  define DSRV_MSG_CHAT_LIST          ((master_request_t)0x39)
#  define DSRV_MSG_CHAT_USER_GET      ((master_request_t)0x3A)
#  define DSRV_MSG_CHAT_PUBLICS       ((master_request_t)0x3B)
#  define DSRV_MSG_CHAT_KICK          ((master_request_t)0x3C)
#  define DSRV_MSG_CHAT_KICKED        ((master_request_t)0x3D)
#  define DSRV_MSG_CHAT_OP            ((master_request_t)0x3E)
#  define DSRV_MSG_CHAT_OPED          ((master_request_t)0x3F)
#  define DSRV_MSG_CHAT_DEOP          ((master_request_t)0x40)
#  define DSRV_MSG_CHAT_DEOPED        ((master_request_t)0x41)
#  define DSRV_MSG_CHAT_LIST_MY_CHANS ((master_request_t)0x42)
#  define DSRV_MSG_CHAT_DESTROY_CHAN  ((master_request_t)0x43)
#  define DSRV_MSG_MAX                ((master_request_t)0x44)
#  define DSRV_MSG_NONE               ((master_request_t)0xFF)

inline bool isPacketHeaderValid(const fts_packet_hdr_t *in_pHdr)
{
    if(!in_pHdr)
        return false;

    // check the package ID.
    return in_pHdr->ident[0] == 'F' &&
           in_pHdr->ident[1] == 'T' &&
           in_pHdr->ident[2] == 'S' &&
           in_pHdr->ident[3] == 'S' &&
           in_pHdr->req_id < DSRV_MSG_MAX;
}

inline bool fillPacketHeader(fts_packet_hdr_t *in_pHdr, master_request_t in_cReq)
{
    if(!in_pHdr)
        return false;

    in_pHdr->ident[0] = 'F';
    in_pHdr->ident[1] = 'T';
    in_pHdr->ident[2] = 'S';
    in_pHdr->ident[3] = 'S';
    in_pHdr->req_id = in_cReq;
    return true;
}

#endif /*FTS_PACKETHEADER_H*/

 /* EOF */
