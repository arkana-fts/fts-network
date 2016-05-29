
#include <string>
#include <ostream>
#include <iostream>
#include <vector>
#include <utility>

#include "Logger.h"

namespace FTS {

int Logger::dbg_level = 0;
std::recursive_mutex Logger::mtx;
std::ostream* Logger::outstream = nullptr;

}