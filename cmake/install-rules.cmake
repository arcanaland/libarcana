# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    TARGETS arcana
    EXPORT arcana-targets
    FILE_SET HEADERS
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

install(
    EXPORT arcana-targets
    FILE arcanaTargets.cmake
    NAMESPACE arcana::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/arcana
  )

install(
    PACKAGE_INFO arcana
    EXPORT arcana-targets
    VERSION ${PROJECT_VERSION}
    DEFAULT_TARGETS arcana
    DESCRIPTION "Shared core for Arcana Land's Tarot tooling"
    HOMEPAGE_URL "https://github.com/arcanaland/libarcana"
  )

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/arcanaConfig.cmake.in
    ${PROJECT_BINARY_DIR}/arcanaConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/arcana
  )

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/arcanaConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

install(
    FILES
        ${PROJECT_BINARY_DIR}/arcanaConfig.cmake
        ${PROJECT_BINARY_DIR}/arcanaConfigVersion.cmake
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/arcana
  )
