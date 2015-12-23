#ifndef CONNECTION_WAITER_H
#define CONNECTION_WAITER_H

#include <functional>
#include "connection.h"

namespace FTS {

class ConnectionWaiter {
public:
    enum class ConnectionType
    {
        SOCKET
    };
    virtual ~ConnectionWaiter() {};
    static ConnectionWaiter* create(ConnectionType t);
    virtual int init(std::uint16_t in_usPort, std::function<void(Connection*)> in_cb) = 0;
    virtual bool waitForThenDoConnection(std::int64_t in_ulMaxWaitMillisec = FTSC_TIME_OUT) = 0;

protected:
    ConnectionWaiter() = default;
};

} // namespace FTSSrv2

#endif