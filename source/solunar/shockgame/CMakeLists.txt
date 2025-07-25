
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SHOCKGAME_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SHOCKGAME_SRC})

add_library(shockgame STATIC
	${SHOCKGAME_SRC}
)
	
target_precompile_headers(shockgame PRIVATE "shockgamepch.h")
target_include_directories(shockgame PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../"
								PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
								
target_link_libraries(shockgame engine graphics game)