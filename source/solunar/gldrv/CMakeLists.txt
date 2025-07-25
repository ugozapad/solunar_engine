set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB GLDRV_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GLDRV_SRC})

add_library(gldrv STATIC ${GLDRV_SRC})

target_precompile_headers(gldrv PRIVATE "gldrv_pch.h")

target_include_directories(gldrv PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../"
								 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../graphics"
								 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
								 PRIVATE "${CMAKE_SOURCE_DIR}/externals/include")

target_link_libraries(gldrv core glad opengl32)