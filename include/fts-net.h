/**
 * \author xkbeyer
 * \date 3. Januar 2016
 * \brief This file contains the interface/setup for the fts-net library
 **/

#pragma once
#include <ostream>

namespace FTS {
    bool NetworkLibInit(int dbgLevel = 0, std::ostream* out = nullptr);
}

 /* EOF */
