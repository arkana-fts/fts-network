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

if(MSVC)
    FOREACH(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        )
        IF(${flag_var} MATCHES "/MD")
            STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        ENDIF()
    ENDFOREACH(flag_var)
endif()

set( src ./src/fts-net.cpp ./src/connection.cpp ./src/TraditionalConnection.cpp ./src/packet.cpp ./src/Logger.cpp ./src/socket_connection_waiter.cpp ./src/connection_waiter.cpp)
set( src_h ./src/TraditionalConnection.h ./src/socket_connection_waiter.h )
set( hdr ./include/fts-net.h ./include/connection.h ./include/packet.h ./include/packet_header.h ./include/Logger.h ./include/TextFormatting.h ./include/dsrv_constants.h ./include/connection_waiter.h)

add_library(fts-net STATIC ${hdr} ${src_h} ${src} )
target_include_directories(fts-net PUBLIC ${PROJECT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_definitions(fts-net PRIVATE _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)
    source_group( Source FILES ${src})
    source_group( Header FILES ${hdr})
    source_group( InternalHeader FILES ${src_h})
elseif(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_definitions(fts-net PRIVATE permissive)
endif()

set_property(TARGET fts-net PROPERTY CXX_STANDARD 14)
set_property(TARGET fts-net PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET fts-net PROPERTY DEBUG_POSTFIX "_d")
set_property(TARGET fts-net PROPERTY ARCHIVE_OUTPUT_DIRECTORY "${fts-networking_SOURCE_DIR}/lib")

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/  DESTINATION include)
install(TARGETS fts-net ARCHIVE DESTINATION lib)
