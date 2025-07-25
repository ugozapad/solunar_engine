set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB GRAPHICS_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB GRAPHICS_CORE_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/core/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp"
)

file(GLOB GRAPHICS_MATERIALS_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/materials/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/materials/*.cpp"
)

file(GLOB GRAPHICS_UI_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/ui/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ui/*.cpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GRAPHICS_SRC})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GRAPHICS_CORE_SRC})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GRAPHICS_MATERIALS_SRC})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GRAPHICS_UI_SRC})

add_library(graphics STATIC
	${GRAPHICS_SRC}
	${GRAPHICS_CORE_SRC}
	${GRAPHICS_MATERIALS_SRC}
	${GRAPHICS_UI_SRC}
)

target_precompile_headers(graphics PRIVATE "graphicspch.h")
target_include_directories(graphics PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../"
				    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_include_directories(graphics PRIVATE "${CMAKE_SOURCE_DIR}/externals/assimp/include")
target_include_directories(graphics PRIVATE "${CMAKE_SOURCE_DIR}/externals/cgltf-1.14")

target_link_libraries(graphics imgui imguizmo core)