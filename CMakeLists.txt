cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

add_compile_options(-Wall)

aux_source_directory(. ALL_SRC)

add_executable(test ${ALL_SRC})

target_link_libraries(test pthread)
