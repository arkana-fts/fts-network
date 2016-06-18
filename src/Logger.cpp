
#include <string>
#include <ostream>
#include <iostream>
#include <vector>
#include <utility>

#include "Logger.h"

namespace FTS {
namespace LoggerImpl {
std::recursive_mutex mtx;
}

int Logger::dbg_level = 0;
std::ostream* Logger::outstream = nullptr;
void Logger::Lock() { LoggerImpl::mtx.lock(); }
void Logger::Unlock() { LoggerImpl::mtx.unlock(); }

}