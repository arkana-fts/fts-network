
project(fts-network-test)

#set(CMAKE_VERBOSE_MAKEFILE ON)

# Setup the make environement. #
################################
cmake_minimum_required(VERSION 3.1)


# Setup the cmake directory containing numerous cmake scripts and macros.
set(CMAKE_MODULE_PATH "${fts-network-test_SOURCE_DIR}/cmake")
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

# Disallow in-source builds. #
##############################
EXECUTE_PROCESS(COMMAND pwd OUTPUT_VARIABLE CURR_DIR)
if("${CURR_DIR}" STREQUAL "${fts-network-test_SOURCE_DIR}\n")
    message(FATAL_ERROR "In-source-builds are not allowed to build the Arkana-FTS network lib. Please go into the \"build\" directory and type \"cmake ..\" there.\nThank you.")
endif()

if(MSVC)
    FOREACH(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE)
        IF(${flag_var} MATCHES "/MD")
            STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        ENDIF()
    ENDFOREACH(flag_var)
endif()

# Define all sourcefiles. #
###########################
set(TEST_SRC packet_test.cpp TextFormatting_test.cpp Logger_test.cpp)
set(HDR catch.hpp ../include/packet.h ../include/TextFormatting.h ../include/Logger.h) 
set(SRC ../src/packet.cpp ../src/Logger.cpp) 
   
if(MSVC)
    source_group( Header FILES ${HDR})
    source_group( Source FILES ${TEST_SRC})
    source_group( Imported\ Source FILES ${SRC})
endif()

# Compiler-dependent and build-dependend flags:
if( NOT MSCV)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_definitions(-D_DEBUG)
    endif()
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
endif()
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()
 
# The compiling process. #
##########################
include_directories( ../include )
add_executable(fts-network-test ${TEST_SRC} ${SRC} ${HDR})
set_property(TARGET fts-network-test PROPERTY CXX_STANDARD 14)
set_property(TARGET fts-network-test PROPERTY CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_custom_command(TARGET fts-network-test POST_BUILD
                       COMMAND $(TargetPath) ARGS "-s" COMMENT "Run the test suite")
else(MSVC)
    add_custom_command(TARGET fts-network-test POST_BUILD
                       COMMAND fts-network-test ARGS "-s" COMMENT "Run the test suite")
endif(MSVC)

#target_link_libraries(fts-network-test)
