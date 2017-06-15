#include "catch.hpp"
#include "../include/Logger.h"
#include <sstream>

using namespace FTS;

TEST_CASE("Message Simple String", "[FTSMSG]")
{
    std::stringstream logout;
    Logger::LogFile(&logout);
    Logger::DbgLevel(5);
    FTSMSG("Testlog", MsgType::Message);
    REQUIRE(logout.str() == "Testlog\n");
}

TEST_CASE("Message one integer parameter", "[FTSMSG]")
{
    std::stringstream logout;
    Logger::LogFile(&logout);
    Logger::DbgLevel(5);
    FTSMSG("Testlog {1}", MsgType::Message, toString(123));
    REQUIRE(logout.str() == "Testlog 123\n");
}

TEST_CASE("Message three parameter", "[FTSMSG]")
{
    std::stringstream logout;
    Logger::LogFile(&logout);
    Logger::DbgLevel(5);
    FTSMSG("Testlog {1} {3} {2}", MsgType::Message, toString(123), toString(123,0,' ',std::ios::hex), "=");
    REQUIRE(logout.str() == "Testlog 123 = 7b\n");
}

TEST_CASE("Warning three parameter", "[FTSMSG]")
{
    std::stringstream logout;
    Logger::LogFile(&logout);
    Logger::DbgLevel(5);
    FTSMSG("Testlog {1} {3} {2}", MsgType::Warning, toString(123), toString(123, 0, ' ', std::ios::hex), "=");
    REQUIRE(logout.str() == "Testlog 123 = 7b\n");
}

TEST_CASE("Error message", "[FTSMSG]")
{
    std::stringstream logout;
    Logger::LogFile(&logout);
    Logger::DbgLevel(5);
    FTSMSG("Testlog {1}", MsgType::Error, toString(1));
    REQUIRE(logout.str() == "Testlog 1\n");
}


TEST_CASE("Messing missing first parameter placeholder", "[FTSMSG]")
{
    std::stringstream logout;
    Logger::LogFile(&logout);
    Logger::DbgLevel(5);
    FTSMSG("Testlog {3} {2}", MsgType::Warning, toString(123), toString(123, 0, ' ', std::ios::hex), "=");
    REQUIRE(logout.str() == "Testlog = 7b\n");
}

TEST_CASE("Debug message same level", "[FTSMSGDBG]")
{
    std::stringstream logout;
    Logger::LogFile(&logout);
    Logger::DbgLevel(1);
    FTSMSGDBG("Testlog {1} {3} {2}", 1, toString(123), toString(123, 0, ' ', std::ios::hex), "=");
    REQUIRE(logout.str() == "Testlog 123 = 7b\n");
}

TEST_CASE("Debug message higher level", "[FTSMSGDBG]")
{
    std::stringstream logout;
    Logger::LogFile(&logout);
    Logger::DbgLevel(1);
    FTSMSGDBG("Testlog {1} {3} {2}", 2, toString(123), toString(123, 0, ' ', std::ios::hex), "=");
    REQUIRE(logout.str().empty());
}

TEST_CASE("Debug message lower level", "[FTSMSGDBG]")
{
    std::stringstream logout;
    Logger::LogFile(&logout);
    Logger::DbgLevel(2);
    FTSMSGDBG("Testlog {1} {3} {2}", 1, toString(123), toString(123, 0, ' ', std::ios::hex), "=");
    REQUIRE(logout.str() == "Testlog 123 = 7b\n");
}
