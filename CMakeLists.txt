cmake_minimum_required(VERSION 3.4.1)

project(RDT3.0)
set(dir ${CMAKE_CURRENT_SOURCE_DIR}/Compile/)
set(src ${CMAKE_CURRENT_SOURCE_DIR}/src/)
set(CMAKE_BUILD_DIRECTORY ${dir})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${dir})
set(CMAKE_BUILD_FILES_DIRECTORY ${dir})
set(CMAKE_BUILD_DIRECTORY ${dir})
set(CMAKE_BINARY_DIR  ${dir})

aux_source_directory(./src SRC_LIST)
add_library(main ${SRC_LIST})

add_executable(altBit ${src}/altBit.c)
add_executable(goBackN ${src}/goBackN.c)
add_executable(selectiveRepeat ${src}/selectiveRepeat.c)