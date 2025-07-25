set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB D3D12DRV_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${D3D12DRV_SRC})

add_library(d3d12drv STATIC ${D3D12DRV_SRC})

target_precompile_headers(d3d12drv PRIVATE "d3d12drv_pch.h")

target_include_directories(d3d12drv PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../"
									PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../graphics"
								    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

#target_link_libraries(d3d12drv engine core graphics)
target_link_libraries(d3d12drv core)