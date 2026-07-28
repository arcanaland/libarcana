# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

add_library(arcana)
add_library(arcana::arcana ALIAS arcana)

configure_file(${PROJECT_SOURCE_DIR}/include/arcana/version.hpp.in
               ${PROJECT_BINARY_DIR}/generated/include/arcana/version.hpp @ONLY)

target_sources(
    arcana
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_BINARY_DIR}/generated/include
        FILES
            ${PROJECT_BINARY_DIR}/generated/include/arcana/version.hpp
    PRIVATE
        version.cpp
)

target_compile_features(arcana PUBLIC cxx_std_26)

target_link_libraries(arcana PRIVATE tomlplusplus::tomlplusplus)

set_target_properties(arcana PROPERTIES POSITION_INDEPENDENT_CODE ON)
