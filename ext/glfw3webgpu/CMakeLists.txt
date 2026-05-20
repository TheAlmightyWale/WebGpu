# This is only meant to be included as a subdirectory in another project.
# It assumes that targets 'glfw' and 'webgpu' exist.
# Loot at examples/CMakeLists.txt to see how to use it in a project.

# The glfw3webgpu target
add_library(glfw3webgpu STATIC glfw3webgpu.c)
target_include_directories(glfw3webgpu PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include>)
target_link_libraries(glfw3webgpu PUBLIC glfw webgpu)

if (APPLE)
  target_compile_options(glfw3webgpu PRIVATE -x objective-c)
  target_link_libraries(glfw3webgpu PRIVATE "-framework Cocoa" "-framework CoreVideo" "-framework IOKit" "-framework QuartzCore")
endif ()

include(GNUInstallDirs)

install(TARGETS glfw3webgpu
  EXPORT glfw3webgpu-targets
  FILE_SET HEADERS
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT glfw3webgpu-targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${TREADLE_LIBRARY_NAME}
		NAMESPACE glfw3webgpu::
)
