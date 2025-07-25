cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(glad)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(glad STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/KHR/khrplatform.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c"
)

target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")