cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(lz4)

add_library(lz4 STATIC
	# Source files
	"${CMAKE_CURRENT_SOURCE_DIR}/src/lz4.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/lz4hc.c"
	
	# Include files
	"${CMAKE_CURRENT_SOURCE_DIR}/include/lz4.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/lz4hc.h"
)

target_include_directories(lz4 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")