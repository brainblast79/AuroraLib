# Serial library for communicating with the Aurora

# Check for CMake minimum version
CMAKE_MINIMUM_REQUIRED(VERSION  3.0)

# This project is designed to be built outside the source tree.
PROJECT(NDIAURORALIB)

if(NOT ${CMAKE_CL_64} EQUAL 1)
	message( FATAL_ERROR "You need to select a 64 bit compiler!\nDelete cache and choose a 64 bit compiler." )
endif()

# CMake options
set(CMAKE_COLOR_MAKEFILE ON)
Set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_AUTOMOC ON)
cmake_policy(SET CMP0020 NEW)
#include (GenerateExportHeader)


# Parallel builds and reduce warnings
if(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
    add_definitions(/W0)
    add_definitions(/MP)
endif()

if(MSVC)
	MESSAGE( STATUS "Setting parallel object builds for MSVC" )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0 /MP")
endif()

# Global boost libraries variables
# These variables need to be set to find the libraries correctly
set(Boost_USE_STATIC_LIBS       ON)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    0N)

unset(Boost_INCLUDE_DIR CACHE)
unset(Boost_LIBRARY_DIRS CACHE)

#Find the boost libraries - system,regex,date_time required for asio
FIND_PACKAGE( Boost 1.53.0 COMPONENTS system regex date_time thread chrono atomic)

# Include boost headers directories
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

# Header Files
SET( NDIAURORA_HEADERS serialCommunicator.h serialThread.h )
SET( AURORA_COMMANDS_HEADERS CommandHandling.h Conversions.h APIStructures.h )

# Source Files
SET( NDIAURORA_SOURCES serialCommunicator.cpp serialThread.cpp ${NDIAURORA_HEADERS} )
SET( AURORA_COMMANDS_SOURCES SystemCRC.cpp CommandConstruction.cpp CommandHandling.cpp Conversions.cpp 
		${AURORA_COMMANDS_HEADERS} )
		
# Build from source files
ADD_LIBRARY(NDIAURORALIB STATIC ${NDIAURORA_SOURCES} ${NDIAURORA_HEADERS} ${AURORA_COMMANDS_SOURCES} ${AURORA_COMMANDS_HEADERS} )
install(TARGETS NDIAURORALIB DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/lib)
install(FILES serialThread.h serialCommunicator.h CommandHandling.h Conversions.h APIStructures.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/NDIAuroraLib)
