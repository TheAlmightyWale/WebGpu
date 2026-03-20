# Glaze JSON parsing library
include(FetchContent)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glaze)

# We add an option to enable different settings when developing the app than
# when distributing it.
option(DEV_MODE "Set up development helper settings" ON)

# Add source to this project's executable.
add_executable (Renderer "main.cpp" "webgpu.h" "webgpu.cpp" "Utils.h" "MathDefs.h" "MeshDefs.h" "ObjLoader.h" "ObjLoader.cpp" "Buffer.h" "Buffer.cpp" "Texture.h" "Texture.cpp" "Quad.h" "ImageLoader.h" "ImageLoader.cpp" "QuadRenderPipeline.h" "QuadRenderPipeline.cpp" "Utils.cpp" "Terrain.h" "QuadDefs.h" "Terrain.cpp"  "Chrono.h" "ResourceManager.h" "ResourceDefs.h" "ResourceManager.cpp" "Renderer.h" "GraphicsContext.h" "GraphicsContext.cpp")

target_include_directories(Renderer SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/ext")
target_link_libraries(Renderer PRIVATE glfw webgpu glfw3webgpu glm glaze::glaze)

set_target_properties(Renderer PROPERTIES 
	CXX_STANDARD 23
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON)

if(DEV_MODE)
    # In dev mode, we load resources from the source tree, so that when we
    # dynamically edit resources (like shaders), these are correctly
    # versioned.
    target_compile_definitions(Renderer PRIVATE
        ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Resources"
    )
else()
    # In release mode, we just load resources relatively to wherever the
    # executable is launched from, so that the binary is portable
    target_compile_definitions(Renderer PRIVATE
        ASSETS_DIR="./Resources"
    )
endif()

if (MSVC)
    target_compile_options(Renderer PRIVATE /W4 /Zc:preprocessor)
    target_compile_definitions(Renderer PRIVATE -D_CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(Renderer PRIVATE -Wall -Wextra -pedantic)
endif()

target_copy_webgpu_binaries(Renderer)

# TODO: Add tests and install targets if needed.
