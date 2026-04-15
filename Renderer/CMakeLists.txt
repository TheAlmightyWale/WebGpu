# We add an option to enable different settings when developing the app than
# when distributing it.
option(DEV_MODE "Set up development helper settings" ON)

add_subdirectory("Core")

# Add source to this project's executable.
add_executable (Renderer "main.cpp" "GpuBuffer.h" "GpuBuffer.cpp" "GpuTexture.h" "GpuTexture.cpp" "Quad.h" 
                "QuadRenderPipeline.h" "QuadRenderPipeline.cpp" "Terrain.h" "QuadDefs.h" "Terrain.cpp" 
                 "Renderer.h" "Renderer.cpp" "GraphicsContext.h" "GraphicsContext.cpp" "ShaderLoader.h" "ShaderLoader.cpp")

target_include_directories(Renderer SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/ext")
target_link_libraries(Renderer PRIVATE Core glfw webgpu glfw3webgpu glm)

set_target_properties(Renderer PROPERTIES 
	CXX_STANDARD 23
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON)

if (MSVC)
    target_compile_options(Renderer PRIVATE /W4 /Zc:preprocessor)
    target_compile_definitions(Renderer PRIVATE -D_CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(Renderer PRIVATE -Wall -Wextra -pedantic)
endif()

target_copy_webgpu_binaries(Renderer)

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
