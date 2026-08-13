# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

add_library(arcana-validation OBJECT)
add_library(arcana::validation ALIAS arcana-validation)

set_target_properties(
    arcana-validation
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
)

target_sources(
    arcana-validation
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            assets.hpp
            catalogue.hpp
            context.hpp
            registry.hpp
            checks/ansi.hpp
            checks/backs.hpp
            checks/cards.hpp
            checks/deck.hpp
            checks/ids.hpp
            checks/images.hpp
            checks/names.hpp
            checks/surrogate.hpp
    PRIVATE
        assets.cpp
        catalogue.cpp
        context.cpp
        registry.cpp
        checks/ansi.cpp
        checks/backs.cpp
        checks/cards.cpp
        checks/deck.cpp
        checks/ids.cpp
        checks/images.cpp
        checks/names.cpp
        checks/surrogate.cpp
)

target_compile_features(arcana-validation PUBLIC cxx_std_26)

target_include_directories(arcana-validation PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(arcana-validation PUBLIC tomlplusplus::tomlplusplus)
target_compile_definitions(arcana-validation PUBLIC TOML_EXCEPTIONS=0)
