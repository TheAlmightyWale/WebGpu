# Glaze JSON parsing library
include(FetchContent)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glaze)

add_library(Core "Utils.h" "Utils.cpp" "MathDefs.h" "MeshDefs.h" "ObjLoader.h" "ObjLoader.cpp" "TextureAtlas.h" 
                "TextureAtlas.cpp" "ImageLoader.h" "ImageLoader.cpp" "Chrono.h" "ResourceManager.h" "ResourceDefs.h"
                "ResourceManager.cpp" "Packing.h" "Packing.cpp" "webgpu.h" "webgpu.cpp")

target_include_directories(Core SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/ext")
target_link_libraries(Core PRIVATE glfw webgpu glfw3webgpu glm glaze::glaze)

set_target_properties(Core PROPERTIES 
	CXX_STANDARD 23
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON)

if (MSVC)
    target_compile_options(Core PRIVATE /W4 /Zc:preprocessor)
    target_compile_definitions(Core PRIVATE -D_CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(Core PRIVATE -Wall -Wextra -pedantic)
endif()

target_copy_webgpu_binaries(Core)