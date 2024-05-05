# CMakeList.txt : CMake project for Renderer, include source and define
# project specific logic here.
#

# We add an option to enable different settings when developing the app than
# when distributing it.
option(DEV_MODE "Set up development helper settings" ON)

# Add source to this project's executable.
add_executable (Renderer "Renderer.cpp" "webgpu.h" "webgpu.cpp" "Utils.h" "MathDefs.h" "MeshDefs.h" "ObjLoader.h" "ObjLoader.cpp" "Buffer.h" "Buffer.cpp" "Texture.h" "Texture.cpp" "Quad.h" "ImageLoader.h" "ImageLoader.cpp")

target_include_directories(Renderer PRIVATE "${CMAKE_SOURCE_DIR}/ext")
target_link_libraries(Renderer PRIVATE glfw webgpu glfw3webgpu glm)

set_target_properties(Renderer PROPERTIES 
	CXX_STANDARD 20
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON)

if(DEV_MODE)
    # In dev mode, we load resources from the source tree, so that when we
    # dynamically edit resources (like shaders), these are correctly
    # versionned.
    target_compile_definitions(Renderer PRIVATE
        ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
    )
else()
    # In release mode, we just load resources relatively to wherever the
    # executable is launched from, so that the binary is portable
    target_compile_definitions(Renderer PRIVATE
        ASSETS_DIR="./resources"
    )
endif()

if (MSVC)
    target_compile_options(Renderer PRIVATE /W4)
else()
    target_compile_options(Renderer PRIVATE -Wall -Wextra -pedantic)
endif()

target_copy_webgpu_binaries(Renderer)

# TODO: Add tests and install targets if needed.
