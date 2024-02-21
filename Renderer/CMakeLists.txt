# CMakeList.txt : CMake project for Renderer, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Renderer "Renderer.cpp" )

target_link_libraries(Renderer PRIVATE glfw webgpu glfw3webgpu)

set_target_properties(Renderer PROPERTIES 
	CXX_STANDARD 20
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON)

if (MSVC)
    target_compile_options(Renderer PRIVATE /W4)
else()
    target_compile_options(Renderer PRIVATE -Wall -Wextra -pedantic)
endif()

target_copy_webgpu_binaries(Renderer)

# TODO: Add tests and install targets if needed.
