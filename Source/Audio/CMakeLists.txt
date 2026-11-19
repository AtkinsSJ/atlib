add_library(Audio STATIC
    AudioPlayer.cpp
    Sound.cpp
)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
find_package(SDL3_mixer REQUIRED)

target_include_directories(Audio PUBLIC ".." ${SDL3_INCLUDE_DIRS} ${SDL3_MIXER_INCLUDE_DIRS})
target_link_libraries(Audio PRIVATE SDL3::SDL3 SDL3_mixer::SDL3_mixer)
