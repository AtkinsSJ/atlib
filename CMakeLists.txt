cmake_minimum_required(VERSION 3.25)
project(AtLib VERSION 0.1)
enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DBUILD_DEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DBUILD_DEBUG=0")

if (MSVC)
    add_compile_options(-EHsc -MD -W4 -wd4201 -Zi -Od -Oi)
else()
    add_compile_options(
        -Wall
        -Werror
        -Wno-exceptions
        -Wno-unknown-pragmas
        -Wno-unqualified-std-cast-call
        -Wundef
    )
endif()

add_library(AtLib INTERFACE)
add_subdirectory("Source")
add_subdirectory("Tests")
include_directories("Source")

target_link_libraries(AtLib INTERFACE App Assets Debug Gfx Input IO Settings UI Util)
