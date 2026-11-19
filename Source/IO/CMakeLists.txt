add_library(IO STATIC
    BinaryFile.cpp
    BinaryFileReader.cpp
    BinaryFileWriter.cpp
    DirectoryIterator.cpp
    DirectoryWatcher.cpp
    File.cpp
    LineReader.cpp
    Paths.cpp
    WriteBuffer.cpp
)
target_include_directories(IO PUBLIC "..")

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

target_include_directories(IO PRIVATE ${SDL3_INCLUDE_DIRS})
target_link_libraries(IO PRIVATE Debug Util ${SDL3_LIBRARIES})
