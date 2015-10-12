# CMakeLists.txt for the fts networkin library
#
# Copyright 2015 by
# Klaus Beyer.
#
# Written by Klaus Beyer <xkbeyer@gmail.com>
#
# This file is part of the Arkana-FTS project, and may only be used, modified,
# and distributed under the terms of the Arkana-FTS project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(fts-networking)

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY   "${fts-networking_SOURCE_DIR}/lib")
set( FTSLIBTYPE "STATIC")
set( CMAKE_DEBUG_POSTFIX "_d")

if(MSVC)
    FOREACH(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        )
        IF(${flag_var} MATCHES "/MD")
            STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        ENDIF()
    ENDFOREACH(flag_var)
else()
    add_definitions(-fpermissive)
endif()
set( FTS_DIR "" CACHE PATH "Path to the FTS root src directory" )

set( src ./src/connection.cpp ./src/packet.cpp ./src/Logger.cpp)
set( hdr ./include/connection.h ./include/packet.h ./include/packet_header.h ./include/Logger.h ./include/TextFormatting.h)
set( FTS_UTIL_SRC ${FTS_DIR}/utilities/DataContainer.cpp ${FTS_DIR}/utilities/threading.cpp)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${FTS_DIR})
add_definitions(-DD_NOCEGUI)

if(MSVC)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
source_group( Source FILES ${src})
source_group( Header FILES ${hdr})
source_group( Utility FILES ${FTS_UTIL_SRC})
endif()

add_library(fts-net STATIC ${hdr} ${src} ${FTS_LOGGER_SRC} ${FTS_UTIL_SRC})
set_property(TARGET fts-net PROPERTY CXX_STANDARD 14)
set_property(TARGET fts-net PROPERTY CXX_STANDARD_REQUIRED ON)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/  DESTINATION include)
install(TARGETS fts-net ARCHIVE DESTINATION lib)
