#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../include/packet.h"
#include <memory>

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
