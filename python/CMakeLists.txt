# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

option(ARCANA_INSTALL_PYTHON_MODULE "Install the arcana_tarot Python package" OFF)

# Development.SABIModule is for nanobind's STABLE_ABI
find_package(
    Python 3.12
    REQUIRED
    COMPONENTS Interpreter Development.Module Development.SABIModule
  )

# nanobind ships its CMake in its Python package
execute_process(
    COMMAND ${Python_EXECUTABLE} -m nanobind --cmake_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE arcana_nanobind_cmake_dir
    COMMAND_ERROR_IS_FATAL ANY
  )

find_package(nanobind CONFIG REQUIRED PATHS ${arcana_nanobind_cmake_dir} NO_DEFAULT_PATH)

set(arcana_python_package arcana_tarot)
set(arcana_python_root ${CMAKE_CURRENT_BINARY_DIR}/import-root)
set(arcana_python_package_dir ${arcana_python_root}/${arcana_python_package})

nanobind_add_module(arcana-python NB_STATIC STABLE_ABI arcana_module.cpp)

set_target_properties(
    arcana-python
    PROPERTIES
        OUTPUT_NAME _core
        LIBRARY_OUTPUT_DIRECTORY ${arcana_python_package_dir}
  )
target_link_libraries(arcana-python PRIVATE arcana::arcana)

configure_file(
    ${arcana_python_package}/__init__.py
    ${arcana_python_package_dir}/__init__.py
    COPYONLY
  )

if(ARCANA_INSTALL_PYTHON_MODULE)
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
                ARCANA_EXAMPLES_DIR=${PROJECT_SOURCE_DIR}/examples/python
                ARCANA_REFERENCE_DECKS_DIR=${arcana_reference_decks_dir}
            ${Python_EXECUTABLE} -m pytest -q ${CMAKE_CURRENT_SOURCE_DIR}/tests
      )

    set_tests_properties(python_smoke PROPERTIES LABELS python)
endif()
