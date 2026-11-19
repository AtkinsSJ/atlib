add_library(Gfx STATIC
    AnimatedSprite.cpp
    AssetLoader.cpp
    BitmapFont.cpp
    Camera.cpp
    Colour.cpp
    Cursor.cpp
    DrawRectsGroup.cpp
    DrawRingsGroup.cpp
    Ninepatch.cpp
    OpenGL.cpp
    Palette.cpp
    Placeholders.cpp
    RenderBuffer.cpp
    Renderer.cpp
    Shader.cpp
    Sprite.cpp
    TextDocument.cpp
    Texture.cpp
)
target_include_directories(Gfx PUBLIC "..")

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
find_package(SDL3_image REQUIRED)
target_include_directories(Gfx PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${SDL3_INCLUDE_DIRS} ${SDL3_IMAGE_INCLUDE_DIRS})

target_link_libraries(Gfx PRIVATE Assets Debug IO Settings Util
    ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${SDL3_LIBRARIES} SDL3_image::SDL3_image)
