#if !defined(LOGGER_H)
#define LOGGER_H

#include <string>
#include <ostream>
#include <iostream>
#include <vector>
#include <utility>
#include <mutex>

#include "TextFormatting.h"


namespace FTS {

/// The types of messages you can send.
/** The types of messages you can send via CFTS::Message or better using macros
*  like the FTSMSG one.
**/
enum class MsgType
{
    /** Print out a warning. That is when something strange occurs but
    *  The game should continue running.
    **/
    Warning,

    /** Prints out an error message, that means something that is fatal
    *  to the normal game so it can't continue.
    **/
    Error,

    /** HORRORs are (catched) developer errors or very fatal (and rare ?)
    *  errors. They can be, for example, invalid parameters to a function.
    **/
    Horror,

    /// This is a positive massage that should be displayed in green within a dialogbox.
    GoodMessage,
    /// This is just a simple message that gets displayed in the console and logfile.
    Message,
    /// This is just a simple message that gets displayed only in console.
    Raw
};

class Logger
{
public:
    Logger() = delete;
    static void DbgLevel( int lvl ) { dbg_level = lvl; }
    static int DbgLevel() { return dbg_level; }
    static void LogFile(std::ostream * out) { outstream = out; }
    static std::ostream& out() { return outstream == nullptr ? std::cout : *outstream; }
    static void Lock();
    static void Unlock();
private:
    static int dbg_level;
    static std::ostream* outstream;
};

template<typename... Ts>
void FTSMSGDBG( std::string in_Msg, int in_iDbgLv, Ts&&... params )
{
    std::vector<std::string> args{ std::forward<Ts>(params)... };
    int i = 1;
    for( auto p : args ) {
        std::string fmt = "{" + toString( i++ ) + "}";
        auto pos = in_Msg.find( fmt );
        if( pos != std::string::npos ) {
            in_Msg.replace( pos, fmt.size(), p );
        }
    }
    FTSMSGDBG( in_Msg, in_iDbgLv );
}

template<>
inline void FTSMSGDBG( std::string in_Msg, int in_iDbgLv )
{
    if( in_iDbgLv <= Logger::DbgLevel() ) {
        Logger::Lock();
        Logger::out() << in_Msg << std::endl;
        Logger::Unlock();
    }
}

template<typename... Ts>
void FTSMSG( std::string in_Msg, FTS::MsgType in_Gravity, Ts&&... params )
{
    std::vector<std::string> args { std::forward<Ts>(params)... };
    int i = 1;
    for( auto p : args ) {
        std::string fmt = "{" + toString( i++ ) + "}";
        auto pos = in_Msg.find( fmt );
        if( pos != std::string::npos ) {
            in_Msg.replace( pos, fmt.size(), p );
        }
    }
    FTSMSG( in_Msg, in_Gravity );
}

template<>
inline void FTSMSG( std::string in_Msg, FTS::MsgType in_Gravity )
{
    Logger::Lock();
    Logger::out() << in_Msg << std::endl;
    Logger::Unlock();
}

} // namespace FTS;
#endif