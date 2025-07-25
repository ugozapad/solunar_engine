set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB D3D11DRV_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${D3D11DRV_SRC})

add_library(d3d11drv STATIC ${D3D11DRV_SRC})

target_precompile_headers(d3d11drv PRIVATE "d3d11drv_pch.h")

target_include_directories(d3d11drv PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../"
									PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../graphics"
								    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

#target_link_libraries(d3d11drv engine core graphics)
target_link_libraries(d3d11drv core)