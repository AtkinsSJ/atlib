add_library(App STATIC
        App.cpp
)
target_include_directories(App PUBLIC "..")
target_link_libraries(App PRIVATE UI Util)
