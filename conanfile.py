from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeConfigDeps

class Renderer(ConanFile):
    name = "renderer"
    version = "0.1"

    # Optional metadata
    license = ""
    author = "Jarryd Peterson"
    url = "https://github.com/TheAlmightyWale/WebGpu"
    description = "2D renderer using WebGpu"
    topics = ("WebGpu", "Graphics", "2D")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "Renderer/*", "Tests/*", "Renderer-ex/*", "ext/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def requirements(self):
        self.requires("glm/1.0.1")
        self.requires("glfw/3.4")
        # Glaze JSON parsing library
        self.requires("glaze/7.4.0")
        
        self.test_requires("gtest/1.17.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeConfigDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["Renderer"]