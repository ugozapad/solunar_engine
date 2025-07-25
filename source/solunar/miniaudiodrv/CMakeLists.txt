set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB MINIAUDIODRV_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${MINIAUDIODRV_SRC})

add_library(miniaudiodrv STATIC ${MINIAUDIODRV_SRC})

target_precompile_headers(miniaudiodrv PRIVATE "miniaudiodrv_pch.h")

target_include_directories(miniaudiodrv PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../"
										PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
										PRIVATE "${CMAKE_SOURCE_DIR}/externals/miniaudio")


target_link_libraries(miniaudiodrv engine core)
