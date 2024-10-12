cmake_minimum_required(VERSION 3.16)

project("mouserot" CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(YAML_CPP_BUILD_CONTRIB OFF)
set(YAML_CPP_BUILD_TOOLS OFF)
set(YAML_CPP_INSTALL OFF)
add_subdirectory(extern/yaml-cpp yaml-cpp)

# makes a shared object from source files
add_executable(mouserot
    src/cli.cpp
    src/mouserot.cpp
    src/utils.h
)

# submodule builds
target_include_directories(mouserot PRIVATE
    extern/clipp/include
    extern/spdlog/include
)

target_link_libraries(mouserot PRIVATE
    evdev
    yaml-cpp::yaml-cpp
)

target_compile_options(mouserot PRIVATE
    -Wall
    -Wextra
    -Wconversion
    # -Wsign-conversion
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g3 -Og)
    add_compile_definitions(
        DEBUG
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3)
    add_compile_definitions(
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO
    )
endif()

install(TARGETS mouserot RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})