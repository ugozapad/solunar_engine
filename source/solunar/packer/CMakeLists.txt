set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(packer "packer.cpp" "packer.h")
set_target_name(packer)
set_target_out_dir(packer "${CMAKE_SOURCE_DIR}/bin")

target_include_directories(packer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(packer lz4)
target_compile_features(packer PRIVATE cxx_std_17)