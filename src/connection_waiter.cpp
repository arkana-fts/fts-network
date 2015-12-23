#include "connection_waiter.h"
#include "socket_connection_waiter.h"

namespace FTS {

FTS::ConnectionWaiter * FTS::ConnectionWaiter::create( ConnectionWaiter::ConnectionType t )
{
    return new SocketConnectionWaiter();
}

}