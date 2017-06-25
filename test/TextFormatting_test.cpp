
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

TEST_CASE("toLower", "[TextFormatting]")
{
    string s = "TestUpperLower";
    s = toLower(s);
    REQUIRE( string("testupperlower") == s);

    s = toLower("test UPper");
    REQUIRE(string("test upper") == s);

    // Can't use german words here, locale isn't correct in this environment.
    //s = toLower( u8"Überhaupt ß" );
    //REQUIRE( string(u8"überhaupt ß") == s);
}

TEST_CASE("toString", "[TextFormatting]")
{
    auto s = toString(5);
    REQUIRE(string("5") == s);

    s = toString(15, 0, ' ', std::ios::hex);
    REQUIRE(string("f") == s);

    s = toString(15, -1, ' ', std::ios::hex);
    REQUIRE(string("f") == s);

    s = toString(0x25, 3, ' ', std::ios::hex);
    REQUIRE(string(" 25") == s);
    s = toString(0xAF, 4, '0', std::ios::hex);
    REQUIRE(string("00af") == s);
    s = toString(1.25f);
    REQUIRE(string("1.25") == s);
    s = toString(3.425);
    REQUIRE(string("3.425") == s);
    std::uint8_t b = 33;
    s = toString(b, -1, ' ', std::ios::hex);
    REQUIRE(string("21") == s);
}

TEST_CASE("trimLeft", "[TextFormatting]")
{
    string s = trim_left_inplace("   test ");
    REQUIRE(string("test ") == s);
    s = trim_left_inplace("\t   test ");
    REQUIRE(string("test ") == s);
    s = trim_left_inplace("\t   test foo");
    REQUIRE(string("test foo") == s);
    s = trim_left_inplace("\r \t  test foo", "\t\r ");
    REQUIRE(string("test foo") == s);
}
TEST_CASE("trimRight", "[TextFormatting]")
{
    string s = trim_right_inplace(" test ");
    REQUIRE(string(" test") == s);
    s = trim_right_inplace("test\t ");
    REQUIRE(string("test") == s);
    s = trim_right_inplace("test foo");
    REQUIRE(string("test foo") == s);
    s = trim_right_inplace("test foo \t \r", "\t\r ");
    REQUIRE(string("test foo") == s);
    s = trim_right_inplace("test foo \r \t ", "\t\r ");
    REQUIRE(string("test foo") == s);
}

TEST_CASE("trim", "[TextFormatting]")
{
    string s = trim(" test ");
    REQUIRE(string("test") == s);
    s = trim("test\t ");
    REQUIRE(string("test") == s);
    s = trim(" test foo ");
    REQUIRE(string("test foo") == s);
    s = trim("\ttest foo \t \r", "\t\r ");
    REQUIRE(string("test foo") == s);
    s = trim(" \r test foo \r \t ", "\t\r ");
    REQUIRE(string("test foo") == s);
}

TEST_CASE("ieq", "[TextFormatting]")
{
    string s1 = "Foo";
    string s2 = "Bar";
    REQUIRE(ieq(s1, s2) == false);

    s1 = "Foo";
    s2 = "Foo";
    REQUIRE(ieq(s1, s2) == true);

    s1 = "Foo";
    s2 = " Foo";
    REQUIRE(ieq(s1, s2) == false);

    s1 = "Foo";
    s2 = "foo";
    REQUIRE(ieq(s1, s2) == true);
}
