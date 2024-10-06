cmake_minimum_required(VERSION 3.16)

project("mouserot" CXX)

set(CMAKE_CXX_STANDARD 20)

# general folders
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(EXTERN_DIR ${CMAKE_SOURCE_DIR}/extern)

# makes a shared object from source files
add_executable(mouserot
    ${SRC_DIR}/cli.cpp
    ${SRC_DIR}/mouserot.cpp
    ${SRC_DIR}/utils.h
)

# submodule builds
target_include_directories(mouserot PRIVATE
    ${EXTERN_DIR}/clipp/include
    ${EXTERN_DIR}/spdlog/include
)

target_link_libraries(mouserot PRIVATE evdev)