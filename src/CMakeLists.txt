configure_file(${CMAKE_SOURCE_DIR}/include/arcana/version.hpp.in
               ${CMAKE_BINARY_DIR}/generated/include/arcana/version.hpp @ONLY)

add_library(arcana version.cpp)
add_library(arcana::arcana ALIAS arcana)

target_sources(
    arcana
    PUBLIC FILE_SET HEADERS
           BASE_DIRS ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/generated/include
           FILES ${CMAKE_BINARY_DIR}/generated/include/arcana/version.hpp
)

target_compile_features(arcana PUBLIC cxx_std_26)

target_link_libraries(arcana PRIVATE tomlplusplus::tomlplusplus)

set_target_properties(arcana PROPERTIES POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    TARGETS arcana
    EXPORT arcana-targets
    FILE_SET HEADERS
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
    EXPORT arcana-targets
    FILE arcanaTargets.cmake
    NAMESPACE arcana::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/arcana)

install(
    PACKAGE_INFO arcana
    EXPORT arcana-targets
    VERSION ${PROJECT_VERSION}
    DEFAULT_TARGETS arcana
    DESCRIPTION "Shared core for Arcana Land tarot-deck tooling"
    HOMEPAGE_URL "https://github.com/arcanaland/libarcana")

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/arcanaConfig.cmake.in
    ${CMAKE_BINARY_DIR}/arcanaConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/arcana)

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/arcanaConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_BINARY_DIR}/arcanaConfig.cmake ${CMAKE_BINARY_DIR}/arcanaConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/arcana)
