add_library(Input STATIC
        Input.cpp
        KeyboardShortcut.cpp
)
target_include_directories(Input PUBLIC "..")
target_link_libraries(Input PRIVATE Debug Gfx Util)
