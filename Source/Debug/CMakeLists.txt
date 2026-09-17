add_library(Debug STATIC
        Console.cpp
        Debug.cpp
        DebugAssetLoader.cpp
        Keymap.cpp
)
target_include_directories(Debug PUBLIC "..")
target_link_libraries(Debug PRIVATE App Assets Gfx IO Input Settings UI Util)
