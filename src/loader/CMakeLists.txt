# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Reads a deck directory into an arcana::deck.
#
# An OBJECT library rather than STATIC: src/CMakeLists.txt absorbs the objects into
# libarcana with $<TARGET_OBJECTS:>, which keeps `arcana` the single installed and
# exported target. Linking it normally would drag arcana-loader into arcana-targets
# and leak an internal name into the installed package description.
add_library(arcana-loader OBJECT)
add_library(arcana::loader ALIAS arcana-loader)

target_sources(
    arcana-loader
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            document.hpp
            loader.hpp
            names.hpp
            toml_read.hpp
    PRIVATE
        document.cpp
        loader.cpp
        names.cpp
)

target_compile_features(arcana-loader PUBLIC cxx_std_26)

target_include_directories(arcana-loader PUBLIC ${PROJECT_SOURCE_DIR}/include)

# PUBLIC, unlike on `arcana` itself: these headers name toml types, so anything that
# includes them -- the component's own tests -- has to agree with it on toml++ and on
# TOML_EXCEPTIONS. Neither reaches the installed target, which stays toml-free.
target_link_libraries(arcana-loader PUBLIC tomlplusplus::tomlplusplus)
target_compile_definitions(arcana-loader PUBLIC TOML_EXCEPTIONS=0)

set_target_properties(arcana-loader PROPERTIES POSITION_INDEPENDENT_CODE ON)
