# 2D Renderer Using WebGpu (WIP)

## Build Options

### Disabling Tests

By default, tests are built along with the main project. To disable building tests, you can use the `BUILD_TESTS` CMake option:

```bash
# Configure without tests
cmake -B build -DBUILD_TESTS=OFF

# Or to explicitly enable tests (default behavior)
cmake -B build -DBUILD_TESTS=ON
```

#### Roadmap

- Loading in and allowing for multiple animations at once
- Debug text rendering
- sprite/ texture atlasing
