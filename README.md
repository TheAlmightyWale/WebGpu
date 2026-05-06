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

### Build Example Application
By default, an example application is built along with the main library. To Toggle this use

```bash
# Configure without tests
cmake -B build -DBUILD_EXAMPLE=OFF
```

#### Roadmap
- Add indirection to global buffers, so users of library can set their own
- Specify maximum limits in shader and cpu side with a single definition
- fixed point rendering and GPU compute

