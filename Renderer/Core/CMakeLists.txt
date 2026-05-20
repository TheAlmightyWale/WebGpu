# Glaze JSON parsing library
include(FetchContent)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glaze)

add_library(Core)
target_sources(Core 
    PRIVATE 
		"Utils.cpp" "TextureAtlas.cpp" "ImageLoader.cpp" "ResourceManager.cpp" "Packing.cpp" "webgpu.cpp"   
 	PUBLIC
		FILE_SET HEADERS
		FILES
        "Utils.h" "MathDefs.h" "TextureAtlas.h" 
        "ImageLoader.h" "Chrono.h" "ResourceManager.h" "ResourceDefs.h"
        "Packing.h" "webgpu.h" "Window.h"
)

target_include_directories(Core SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/ext")
target_link_libraries(Core PUBLIC glfw webgpu glfw3webgpu glm::glm PRIVATE glaze::glaze)

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

include(GNUInstallDirs)

install(TARGETS Core
		EXPORT core-targets
		FILE_SET HEADERS
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT core-targets
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${TREADLE_LIBRARY_NAME}
		NAMESPACE renderer::		
)