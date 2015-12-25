#if !defined(LOGGER_H)
#define LOGGER_H

#include <string>
#include <ostream>
#include <iostream>
#include <vector>
#include <utility>

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

    /** Print out a warning without displaying a messagebox. That is when
    *  something strange occurs but the game should continue running
    *  without disturbing the player with a messagebox.
    **/
    WarningNoMB,

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
    /// This is just a simple message that gets also displayed in a dialogbox.
    Message,
    /// This is just a simple message that gets displayed in the console and logfile.
    MessageNoMB,
    /// This is just a simple message that gets displayed only in console.
    Raw
};


template<typename T>
void pack_param( std::vector<std::string>& pack, T first )
{
    pack.push_back( first );
}

template<typename T, typename... Ts>
void pack_param( std::vector<std::string>& pack, T first, Ts... params )
{
    pack.push_back( first );
    pack_param( pack, params... );
}

template<typename... Ts>
void FTSMSGDBG( std::string in_Msg, int in_iDbgLv, Ts... params )
{
    std::vector<std::string> args;
    pack_param( args, params... );
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
    std::cout << in_Msg << std::endl;
}

template<typename... Ts>
void FTS18N( std::string in_Msg, FTS::MsgType in_Gravity, Ts... params )
{
    std::vector<std::string> args;
    pack_param( args, params... );
    int i = 1;
    std::string addToTheEnd;
    for( auto p : args ) {
        std::string fmt = "{" + toString( i++ ) + "}";
        auto pos = in_Msg.find( fmt );
        if( pos != std::string::npos ) {
            in_Msg.replace( pos, fmt.size(), p );
        } else {
            // In case there is nor parameter, just concatenate the parameter.
            // This is currently the case since the original has a translation in between which contains the format specifier.
            addToTheEnd += " " + p;
        }
    }
    FTS18N( in_Msg + addToTheEnd, in_Gravity );
}

template<>
inline void FTS18N( std::string in_Msg, FTS::MsgType in_Gravity )
{
    std::cout << in_Msg << std::endl;
}

template<typename... Ts>
void FTSMSG( std::string in_Msg, FTS::MsgType in_Gravity, Ts... params )
{
    std::vector<std::string> args;
    pack_param( args, params... );
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
inline void FTSMSG( std::string in_Msg, FTS::MsgType in_Gravity)
{
    std::cout << in_Msg << std::endl;
}

} // namespace FTS;
#endif