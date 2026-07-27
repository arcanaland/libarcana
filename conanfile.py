from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout


class LibarcanaConan(ConanFile):
    name = "libarcana"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"

    requires = "tomlplusplus/3.4.0"
    test_requires = "catch2/3.15.2"

    def layout(self):
        cmake_layout(self)

    def generate(self):
        toolchain = CMakeToolchain(self, generator="Ninja")
        toolchain.generate()
        CMakeDeps(self).generate()
