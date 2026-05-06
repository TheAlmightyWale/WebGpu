find_package(GTest REQUIRED)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Enable testing
enable_testing()

# Include directories for the main project
include_directories(${CMAKE_SOURCE_DIR}/Renderer)
include_directories(${CMAKE_SOURCE_DIR}/Renderer/Core)

# Create test executable
add_executable(RendererTests
  BlitTests.cpp
  FastFillTests.cpp
)

# Link test executable with Google Test and the main project libraries
target_link_libraries(RendererTests
  GTest::gtest_main
  GTest::gtest
  Core
)

# Set C++ standard
target_compile_features(RendererTests PRIVATE cxx_std_20)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(RendererTests)