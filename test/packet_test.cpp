#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../include/packet.h"
#include <memory>
#include "../include/dsrv_constants.h"

using namespace FTS;
using namespace std;

TEST_CASE("Create ptr", "[Packet]")
{
    auto p = make_unique<Packet>( DSRV_MSG_LOGIN );

    REQUIRE( p != nullptr );
    REQUIRE( p->getType() == DSRV_MSG_LOGIN );
    REQUIRE( p->getPayloadLen() == 0 );
    REQUIRE( p->getPayloadPtr() != nullptr );
    REQUIRE( p->getTotalLen() == D_PACKET_HDR_LEN );
}

TEST_CASE( "Create obj", "[Packet]" )
{
    Packet p( DSRV_MSG_LOGIN );

    REQUIRE( p.getType() == DSRV_MSG_LOGIN );
    REQUIRE( p.getPayloadLen() == 0 );
    REQUIRE( p.getPayloadPtr() != nullptr );
    REQUIRE( p.getTotalLen() == D_PACKET_HDR_LEN );
}

TEST_CASE( "move ctr", "[Packet]" )
{
    Packet p( DSRV_MSG_LOGIN );

    REQUIRE( p.getType() == DSRV_MSG_LOGIN );
    REQUIRE( p.getPayloadLen() == 0 );
    REQUIRE( p.getPayloadPtr() != nullptr );
    REQUIRE( p.getTotalLen() == D_PACKET_HDR_LEN );

    p.append( 1234L );
    p.append( "Hello" );
    auto len = (sizeof( long ) + sizeof( "Hello" ));
    REQUIRE( p.getPayloadLen() == len );

    Packet p2( move(p) );
    p2.rewind();
    REQUIRE( p2.getType() == DSRV_MSG_LOGIN );
    REQUIRE( p2.getPayloadPtr() != nullptr );
    REQUIRE( p2.getPayloadLen() == len);
    long v;
    p2.get( v );
    REQUIRE( v == 1234L );
    string s = p2.get_string();
    REQUIRE( s == "Hello" );
    // Since the data has a nullptr it results in the offset. 
    REQUIRE( p.getPayloadPtr() == (int8_t*)9 );
}

TEST_CASE( "move assignment", "[Packet]" )
{
    Packet p( DSRV_MSG_LOGIN );

    REQUIRE( p.getType() == DSRV_MSG_LOGIN );
    REQUIRE( p.getPayloadLen() == 0 );
    REQUIRE( p.getPayloadPtr() != nullptr );
    REQUIRE( p.getTotalLen() == D_PACKET_HDR_LEN );

    p.append( 1234L );
    p.append( "Hello" );
    auto len = (sizeof( long ) + sizeof( "Hello" ));
    REQUIRE( p.getPayloadLen() == len );

    Packet p2(DSRV_MSG_MAX);
    p2 = move(p);
    p2.rewind();
    REQUIRE( p2.getType() == DSRV_MSG_LOGIN );
    REQUIRE( p2.getPayloadPtr() != nullptr );
    REQUIRE( p2.getPayloadLen() == len );
    long v;
    p2.get( v );
    REQUIRE( v == 1234L );
    string s = p2.get_string();
    REQUIRE( s == "Hello" );
    // Since the data has a nullptr it results in the offset. 
    REQUIRE( p.getPayloadPtr() == (int8_t*) 9 );
}

TEST_CASE( "Set/Get Type", "[Packet]" )
{
    Packet p( DSRV_MSG_CHAT_KICK );
    REQUIRE( p.getType() == DSRV_MSG_CHAT_KICK );
    p.setType( DSRV_MSG_GAME_INFO );
    REQUIRE( p.getType() == DSRV_MSG_GAME_INFO );
    p.setType( DSRV_MSG_MAX );
    REQUIRE( p.getType() == DSRV_MSG_MAX );
    p.setType( DSRV_MSG_NULL );
    REQUIRE( p.getType() == DSRV_MSG_NULL );
    p.setType( DSRV_MSG_NONE );
    REQUIRE( p.getType() == DSRV_MSG_NONE );
}

TEST_CASE( "isValid", "[Packet]" )
{
    Packet p( DSRV_MSG_FEEDBACK );
    REQUIRE( p.isValid() );

    p.setType( 0 );
    REQUIRE( p.isValid() );

    p.setType( DSRV_MSG_NULL );
    REQUIRE( p.isValid() );

    p.setType( DSRV_MSG_MAX );
    REQUIRE_FALSE( p.isValid() );

    p.setType( DSRV_MSG_NONE );
    REQUIRE_FALSE( p.isValid() );

    // Check w/ knownledge of the internal buffer layout ;)
    auto ptr = p.getPayloadPtr();
    ptr[-9] = 'A'; // The header starts 9 bytes in front.
    REQUIRE_FALSE( p.isValid() );
}

TEST_CASE( "Data in out handling", "[Packet]" )
{
    Packet p( DSRV_MSG_FEEDBACK );
    REQUIRE( p.isValid() );
    REQUIRE( p.getPayloadLen() == 0 );
    REQUIRE( p.getTotalLen() == sizeof(fts_packet_hdr_t) );
    size_t len = 0;
    p.append( 1234 );
    len += sizeof( int );
    REQUIRE( p.getPayloadLen() == len );
    REQUIRE( p.getTotalLen() == sizeof( fts_packet_hdr_t ) + len );

    p.append( "Hallo" );
    len += sizeof( "Hallo" );
    REQUIRE( p.getPayloadLen() == len );
    REQUIRE( p.getTotalLen() == sizeof( fts_packet_hdr_t ) + len );

    p.append( string("Otto") );
    len += string( "Otto" ).length() + 1 ; // add trailing 0
    REQUIRE( p.getPayloadLen() == len );
    REQUIRE( p.getTotalLen() == sizeof( fts_packet_hdr_t ) + len );

    p.append( 'A' );
    len += sizeof( 'A' );
    REQUIRE( p.getPayloadLen() == len );
    REQUIRE( p.getTotalLen() == sizeof( fts_packet_hdr_t ) + len );

    p.append( 1ULL );
    len += sizeof( unsigned long long );
    REQUIRE( p.getPayloadLen() == len );
    REQUIRE( p.getTotalLen() == sizeof( fts_packet_hdr_t ) + len );

    char data[10] = { 1,2,3,4,5,6,7,8,9,0 };
    p.append( data, 10 );
    len += 10;
    REQUIRE( p.getPayloadLen() == len );
    REQUIRE( p.getTotalLen() == sizeof( fts_packet_hdr_t ) + len );

    p.rewind();
    REQUIRE( p.getPayloadLen() == len );
    REQUIRE( p.getTotalLen() == sizeof( fts_packet_hdr_t ) + len );

    int d = 0;
    p.get( d );
    REQUIRE( d == 1234 );

    auto s = p.get_string();
    REQUIRE( s == "Hallo" );

    s = p.get_string();
    REQUIRE( s == "Otto" );

    char a = 0;
    p.get( a );
    REQUIRE( a == 'A' );

    unsigned long long ull = 0;
    p.get( ull );
    REQUIRE( ull == 1ULL );

    char dataread[10];
    p.get( dataread, 10 );
    REQUIRE( memcmp( data, dataread, 10 ) == 0 );
}

TEST_CASE("Extracting a string","[Packet]")
{
    Packet p( DSRV_MSG_LOGOUT );
    REQUIRE( p.isValid() );
    REQUIRE( p.getPayloadLen() == 0 );
    REQUIRE( p.getTotalLen() == sizeof( fts_packet_hdr_t ) );

    p.append( 1234 );
    p.append( "foo" );
    p.append( '0' );
    REQUIRE( p.getPayloadLen() == (sizeof(int) + sizeof("foo")+ sizeof('0')) );
    
    auto s = p.extractString();
    REQUIRE( s == "" );
    REQUIRE( p.getPayloadLen() == (sizeof( int ) + sizeof( "foo" ) + sizeof( '0' )) );
    
    p.rewind();
    int d = 0;
    p.get( d );
    REQUIRE( d == 1234 );

    s = p.extractString();
    REQUIRE( s == "foo" );
    REQUIRE( p.getPayloadLen() == (sizeof( int ) + sizeof( '0' )) );

    char a = 0;
    p.get( a );
    REQUIRE( a == '0' );
}

TEST_CASE( "Extracting a string at wrong postion", "[Packet]" )
{
    Packet p( DSRV_MSG_LOGOUT );
    REQUIRE( p.isValid() );
    REQUIRE( p.getPayloadLen() == 0 );
    REQUIRE( p.getTotalLen() == sizeof( fts_packet_hdr_t ) );

    p.append( 1234 );
    p.append( "foo" );
    p.append( '0' );
    REQUIRE( p.getPayloadLen() == (sizeof( int ) + sizeof( "foo" ) + sizeof( '0' )) );

    p.rewind();
    auto s = p.extractString();
    REQUIRE( s.length() == 2 );
    REQUIRE( (uint8_t)s[0] == 0xd2 );
    REQUIRE( s[1] == 4 );
    REQUIRE( p.getPayloadLen() == (sizeof( int ) + sizeof( "foo" ) + sizeof( '0' ) - 3) );
    // Now all following data meaningless, since the string is removed from the buffer.
}

TEST_CASE( "Extracting a string w/o terminating 0", "[Packet]" )
{
    Packet p( DSRV_MSG_LOGOUT );
    REQUIRE( p.isValid() );
    REQUIRE( p.getPayloadLen() == 0 );
    REQUIRE( p.getTotalLen() == sizeof( fts_packet_hdr_t ) );

    char data[5] = { 'H', 'a', 'l', 'l', 'o' };
    p.append( data, 5 );
    p.append( '0' );
    p.append( 0 ); // Put a 0 to avoid a crash of the app ;)
    REQUIRE( p.getPayloadLen() == (sizeof( data ) + sizeof( '0' ) + sizeof(int) ) );

    p.rewind();
    auto s = p.extractString();
    REQUIRE( s.length() == 6 );
    REQUIRE( s == "Hallo0" );
    REQUIRE( p.getPayloadLen() == (sizeof(int) - 1) );
    // Now all following data meaningless, since the string is removed from the buffer.
}

TEST_CASE( "Transfering data", "[Packet]" )
{
    Packet p( DSRV_MSG_LOGOUT );
    p.append( "Hallo Otto!" );

    Packet p2( DSRV_MSG_CHAT_GETMSG );
    p2.transferData( &p );

    p2.rewind();
    auto s = p2.get_string();

    REQUIRE( s == "Hallo Otto!" );
}

TEST_CASE( "Writing to/ reading from a packet", "[Packet]" )
{
    Packet p( DSRV_MSG_CHAT_GETMSG );
    p.append( "Hallo Otto!" );

    Packet p2( DSRV_MSG_CHAT_MOTTO_GET );
    p.writeToPacket( &p2 );

    Packet p3( DSRV_MSG_NULL );
    p2.rewind();
    p3.readFromPacket( &p2 );

    REQUIRE( p3.getType() == DSRV_MSG_CHAT_GETMSG );
    REQUIRE( p3.getPayloadLen() == p.getPayloadLen() );
    auto s = p3.get_string();
    REQUIRE( s == "Hallo Otto!" );
}

TEST_CASE( "Append non generic data", "[Packet]" )
{
    Packet p( DSRV_MSG_CHAT_GETMSG );
    DSRV_CHAT_TYPE type = DSRV_CHAT_TYPE::NORMAL;
    p.append(type);
    p.rewind();
    type = DSRV_CHAT_TYPE::NONE;
    p.get( type );
    REQUIRE( type == DSRV_CHAT_TYPE::NORMAL);

}