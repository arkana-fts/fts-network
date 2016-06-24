/**
 * \file TextFormatting.h
 * \author Klaus Beyer
 * \date 11 Oct 2015
 * \brief This file has some utility functions for string formatting.
 *        
 **/

#ifndef FTS_TEXT_FORMATTING_H
#define FTS_TEXT_FORMATTING_H

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace FTS {

template <class T>
static inline std::string toString( const T& t, std::streamsize in_iWidth = 0, char in_cFill = ' ', std::ios_base::fmtflags in_fmtfl = std::ios::dec, typename std::enable_if< std::is_arithmetic<T>::value >::type* = 0 )
{
    std::stringstream out;
    out.width( in_iWidth );
    out.fill( in_cFill );
    out.flags( in_fmtfl );
    // Special case for char types and formatting as hex value. 
    if( sizeof( T ) == 1 && in_fmtfl == std::ios::hex ) {
        out << (int)(std::uint8_t) t;
    } else {
        out << t;
    }
    return out.str();
}

static inline std::string toHexString( const char* buf, std::size_t len )
{
    std::string outstring;
    for( size_t i = 0; i < len; ++i ) {
        outstring += toString(buf[i], 2, '0', std::ios::hex);
    }
    return outstring;
}

static inline std::string toLower( const std::string& in_str )
{
    std::string ret = in_str;
    std::transform( ret.begin(), ret.end(), ret.begin(), ::tolower ); // Thanks to SO
    return ret;
}

static inline bool ieq( const std::string& lhs, const std::string& rhs )
{
    return toLower( lhs ) == toLower( rhs );
}

static inline std::string trim_left_inplace( std::string s, const std::string& delimiters = " \t" )
{
    return s.erase( 0, s.find_first_not_of( delimiters ) );
}

static inline std::string trim_right_inplace( std::string s, const std::string& delimiters = " \t" )
{
    return s.erase( s.find_last_not_of( delimiters ) + 1, std::string::npos );
}

static inline std::string trim( std::string s, const std::string& delimiters = " \t" )
{
    return trim_left_inplace( trim_right_inplace( s, delimiters ), delimiters );
}

}

#endif /* FTS_TEXT_FORMATTING_H */

 /* EOF */
