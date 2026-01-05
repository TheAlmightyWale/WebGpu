# Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.25)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
  SYSTEM
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Include directories for the main project
include_directories(${CMAKE_SOURCE_DIR}/Renderer)

# Create test executable
add_executable(RendererTests
  ExampleTest.cpp
)

# Link test executable with Google Test and the main project libraries
target_link_libraries(RendererTests
  GTest::gtest_main
  GTest::gtest
  glm::glm
  glfw
  webgpu
  glfw3webgpu
)

# Set C++ standard
target_compile_features(RendererTests PRIVATE cxx_std_20)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(RendererTests)