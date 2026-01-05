# WebGPU Renderer Unit Tests

This directory contains unit tests for the WebGPU Renderer project using Google Test framework.

## Building the Tests

The tests are automatically built as part of the main project build. From the project root:

```bash
# Configure the project (if not already done)
cmake --preset=default

# Build the project including tests
cmake --build build
```

## Running the Tests

After building, you can run the tests in several ways:

### Using CTest (Recommended)

```bash
# From the build directory
cd build
ctest

# For verbose output
ctest -V

# To run specific tests
ctest -R ExampleTest
```

### Direct Execution

```bash
# From the build directory
./Tests/RendererTests

# Run specific test suites
./Tests/RendererTests --gtest_filter=MathTest.*

# Run specific test
./Tests/RendererTests --gtest_filter=MathTest.Vec3Operations
```

## Test Organization

- **ExampleTest.cpp**: Basic math and transformation tests demonstrating GLM usage

  - Vector and matrix operations
  - Camera transformations
  - 2D sprite transformations
  - Parameterized tests for different sprite sizes

- **UtilsTest.cpp**: Utility function tests
  - Angle conversions
  - Value clamping and interpolation
  - Power of two calculations
  - Color packing/unpacking
  - Texture utilities
  - Performance testing examples (disabled by default)

## Writing New Tests

To add new tests:

1. Create a new `.cpp` file in the Tests directory
2. Include the Google Test header: `#include <gtest/gtest.h>`
3. Write your tests using TEST() or TEST_F() macros
4. Add the new file to the `add_executable()` list in Tests/CMakeLists.txt

### Example Test Structure

```cpp
#include <gtest/gtest.h>
#include "YourHeaderFile.h"

TEST(TestSuiteName, TestName) {
    // Arrange
    int expected = 42;

    // Act
    int result = YourFunction();

    // Assert
    EXPECT_EQ(result, expected);
}
```

## Test Fixtures

For tests that need common setup/teardown:

```cpp
class YourTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup code
    }

    void TearDown() override {
        // Common cleanup code
    }

    // Shared test data
    YourClass testObject;
};

TEST_F(YourTestFixture, TestName) {
    // Use testObject and other fixture members
}
```

## Useful Google Test Assertions

- `EXPECT_EQ(val1, val2)` - Expect values to be equal
- `EXPECT_NE(val1, val2)` - Expect values to be not equal
- `EXPECT_TRUE(condition)` - Expect condition to be true
- `EXPECT_FALSE(condition)` - Expect condition to be false
- `EXPECT_FLOAT_EQ(val1, val2)` - Compare floating point values
- `EXPECT_NEAR(val1, val2, abs_error)` - Compare with tolerance
- `EXPECT_THROW(statement, exception_type)` - Expect exception
- `EXPECT_NO_THROW(statement)` - Expect no exception

Use `ASSERT_*` variants when you want the test to stop on failure.

## Performance Tests

Performance tests are disabled by default. To run them:

```bash
./Tests/RendererTests --gtest_also_run_disabled_tests
```

## Debugging Tests

To debug a specific test:

1. Run with `--gtest_break_on_failure` to break into debugger on failure
2. Use `--gtest_repeat=N` to run tests N times
3. Use `--gtest_shuffle` to randomize test order (helps find dependencies)

## Coverage

To generate code coverage reports (requires appropriate tools):

```bash
# Build with coverage flags
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage" ..
make

# Run tests
./Tests/RendererTests

# Generate coverage report
gcov *.cpp
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```
