add_library(Settings STATIC
        Setting.cpp
        Settings.cpp
        SettingsState.cpp
)
target_include_directories(Settings PUBLIC "..")
target_link_libraries(Settings PRIVATE Assets IO UI Util)
