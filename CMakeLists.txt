# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.30)

# Enable Hot Reload for MSVC compilers if supported.
if (MSVC AND POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Renderer")

find_package(glm CONFIG REQUIRED)
find_package(glfw3 REQUIRED)

# Include sub-projects.
add_subdirectory("ext/webgpu")
add_subdirectory("ext/glfw3webgpu")
add_subdirectory("Renderer")

# Option to enable/disable building tests
option(BUILD_TESTS "Build the test suite" ON)
option(BUILD_EXAMPLE "Build Example Application" ON)

if(BUILD_EXAMPLE)
	add_subdirectory("Renderer-ex")
endif()

# Include tests directory if BUILD_TESTS is enabled
if(BUILD_TESTS)
    add_subdirectory("Tests")
endif()

include(GNUInstallDirs)

install(TARGETS Renderer
		EXPORT renderer-targets
		FILE_SET HEADERS
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT renderer-targets
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/renderer
		NAMESPACE renderer::		
)
