#include "fts-net.h"
#include "Logger.h"

bool FTS::NetworkLibInit( int dbgLevel, std::ostream * out )
{
    Logger::DbgLevel( dbgLevel );
    Logger::LogFile(out);
    return true;
}
