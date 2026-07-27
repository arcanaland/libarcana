find_package(Catch2 3 REQUIRED)
include(Catch)

function(libarcana_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})

    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain ${ARG_LIBS})

    catch_discover_tests(${name})
endfunction()

libarcana_add_test(version_test SOURCES version_test.cpp LIBS arcana::arcana)
