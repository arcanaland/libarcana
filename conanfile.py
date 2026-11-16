# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

import os
import re

from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import load


class LibarcanaConan(ConanFile):
    name = "libarcana"
    settings = "os", "compiler", "build_type", "arch"

    requires = "tomlplusplus/3.4.0"
    test_requires = "catch2/3.15.2"
    exports = "CMakeLists.txt"

    def set_version(self):
        # The same regex pyproject.toml uses: project(VERSION) is the one source
        cmake = load(self, os.path.join(self.recipe_folder, "CMakeLists.txt"))
        match = re.search(r"project\(\s+arcana\s+VERSION\s+([0-9][0-9.]*)", cmake)
        self.version = match.group(1)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        toolchain = CMakeToolchain(self, generator="Ninja")
        toolchain.generate()
        CMakeDeps(self).generate()
