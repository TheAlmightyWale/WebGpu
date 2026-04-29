add_executable(Renderer-ex "main.cpp")

target_include_directories(Renderer-ex SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/ext")
target_include_directories(Renderer-ex PRIVATE "${CMAKE_SOURCE_DIR}/Renderer" "${CMAKE_SOURCE_DIR}/Renderer/Core")
target_link_libraries(Renderer-ex PRIVATE Renderer)

set_target_properties(Renderer-ex PROPERTIES 
	CXX_STANDARD 23
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON)

target_copy_webgpu_binaries(Renderer-ex)