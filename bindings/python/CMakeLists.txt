# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

find_package(Python 3.12 REQUIRED COMPONENTS Interpreter Development.Module)

# nanobind ships its CMake in its Python package
execute_process(
    COMMAND ${Python_EXECUTABLE} -m nanobind --cmake_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE arcana_nanobind_cmake_dir
    COMMAND_ERROR_IS_FATAL ANY
  )

find_package(nanobind CONFIG REQUIRED PATHS ${arcana_nanobind_cmake_dir} NO_DEFAULT_PATH)

nanobind_add_module(arcana-python NB_STATIC arcana_module.cpp)

set_target_properties(arcana-python PROPERTIES OUTPUT_NAME arcana)
target_link_libraries(arcana-python PRIVATE arcana::arcana)

# Where pytest should import the module
set(arcana_python_module_dir $<TARGET_FILE_DIR:arcana-python>)

if(ARCANA_BUILD_TESTS)
    add_test(
        NAME python_smoke
        COMMAND
            ${CMAKE_COMMAND} -E env
                PYTHONPATH=${arcana_python_module_dir}
                ARCANA_FIXTURES_DIR=${PROJECT_SOURCE_DIR}/tests/fixtures
            ${Python_EXECUTABLE} -m pytest -q ${CMAKE_CURRENT_SOURCE_DIR}/tests
      )

    set_tests_properties(python_smoke PROPERTIES LABELS python)
endif()
