# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

option(ARCANA_INSTALL_PYTHON_MODULE "Install the arcana_tarot Python package" OFF)

find_package(Python 3.12 REQUIRED COMPONENTS Interpreter Development.Module)

# nanobind ships its CMake in its Python package
execute_process(
    COMMAND ${Python_EXECUTABLE} -m nanobind --cmake_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE arcana_nanobind_cmake_dir
    COMMAND_ERROR_IS_FATAL ANY
  )

find_package(nanobind CONFIG REQUIRED PATHS ${arcana_nanobind_cmake_dir} NO_DEFAULT_PATH)

# ADR-025: the wheel ships a package `arcana_tarot/` holding exactly one extension
# module, `_core`. Further specs become pure-Python submodules over that surface
# rather than further extensions, so `libarcana.a` is linked in exactly once.
set(arcana_python_package arcana_tarot)
set(arcana_python_root ${CMAKE_CURRENT_BINARY_DIR}/import-root)
set(arcana_python_package_dir ${arcana_python_root}/${arcana_python_package})

nanobind_add_module(arcana-python NB_STATIC arcana_module.cpp)

set_target_properties(
    arcana-python
    PROPERTIES
        OUTPUT_NAME _core
        LIBRARY_OUTPUT_DIRECTORY ${arcana_python_package_dir}
  )
target_link_libraries(arcana-python PRIVATE arcana::arcana)

# The build tree has to hold the same package shape the wheel installs, or pytest
# has nothing to import. This is the seam ADR-025 names as debt: only the venv
# check in `just test-wheel` exercises the real artifact.
configure_file(
    ${arcana_python_package}/__init__.py
    ${arcana_python_package_dir}/__init__.py
    COPYONLY
  )

if(ARCANA_INSTALL_PYTHON_MODULE)
    # Into the wheel's platlib, as a package. Deliberately independent of
    # ARCANA_INSTALL, which the wheel build sets OFF to keep the C++ tree out.
    install(TARGETS arcana-python LIBRARY DESTINATION ${arcana_python_package})
    install(
        FILES ${arcana_python_package}/__init__.py
        DESTINATION ${arcana_python_package}
      )
endif()

if(ARCANA_BUILD_TESTS)
    add_test(
        NAME python_smoke
        COMMAND
            ${CMAKE_COMMAND} -E env
                PYTHONPATH=${arcana_python_root}
                ARCANA_FIXTURES_DIR=${PROJECT_SOURCE_DIR}/tests/fixtures
            ${Python_EXECUTABLE} -m pytest -q ${CMAKE_CURRENT_SOURCE_DIR}/tests
      )

    set_tests_properties(python_smoke PROPERTIES LABELS python)
endif()
