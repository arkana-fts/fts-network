
#include "catch.hpp"
#include "../include/TextFormatting.h"
#include <memory>
#include <iostream>

using namespace FTS;
using namespace std;

TEST_CASE("HexString", "[TextFormatting]")
{
    const char Hex[] = { 1, 0x10, 0x33, 0xfa,0 };
    auto HexStr = toHexString(Hex, 5);
    REQUIRE( HexStr == "011033fa00" );
}

TEST_CASE("int", "[TextFormatting]")
{
    auto HexStr = toString(5000);
    REQUIRE(HexStr == "5000");
}

TEST_CASE("long", "[TextFormatting]")
{
    auto HexStr = toString(5000L);
    REQUIRE(HexStr == "5000");
}

TEST_CASE("longHexDefaultWidth", "[TextFormatting]")
{
    auto HexStr = toString(0x1500L,0,'0',std::ios::hex);
    REQUIRE(HexStr == "1500");
}

TEST_CASE("longHexWidth", "[TextFormatting]")
{
    auto HexStr = toString(0x1500L, 6, '0', std::ios::hex);
    REQUIRE(HexStr == "001500");
}

TEST_CASE("longHexWidthShorter", "[TextFormatting]")
{
    auto HexStr = toString(0x123456L, 4, '0', std::ios::hex);
    REQUIRE(HexStr == "123456");
}
