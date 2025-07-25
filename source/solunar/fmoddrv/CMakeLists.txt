set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB FMODDRV_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${FMODDRV_SRC})

add_library(fmoddrv STATIC ${FMODDRV_SRC})

target_precompile_headers(fmoddrv PRIVATE "fmoddrv_pch.h")

target_include_directories(fmoddrv PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../"
								   PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
								   PRIVATE "${CMAKE_SOURCE_DIR}/externals/fmod/inc"
								   PRIVATE "${CMAKE_SOURCE_DIR}/externals/fmod_studio/inc")

# TODO: for windows only
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
	target_link_directories(fmoddrv PRIVATE "${CMAKE_SOURCE_DIR}/externals/fmod_studio/lib/x86")
	target_link_directories(fmoddrv PRIVATE "${CMAKE_SOURCE_DIR}/externals/fmod/lib/x86")
else()
	target_link_directories(fmoddrv PRIVATE "${CMAKE_SOURCE_DIR}/externals/fmod_studio/lib/x64")
	target_link_directories(fmoddrv PRIVATE "${CMAKE_SOURCE_DIR}/externals/fmod/lib/x64")
endif()

target_link_libraries(fmoddrv engine core)
target_link_libraries (fmoddrv
    debug "fmodstudioL_vc.lib"
    optimized "fmodstudio_vc.lib")
target_link_libraries (fmoddrv
    debug "fmodL_vc.lib"
    optimized "fmod_vc.lib")
