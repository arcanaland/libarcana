find_package(Catch2 3 REQUIRED)
include(Catch)

function(libarcana_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})

    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain ${ARG_LIBS})

    catch_discover_tests(${name})
endfunction()

libarcana_add_test(version_test SOURCES version_test.cpp LIBS arcana::arcana)

# Consumer tests: build against an installed libarcana
if(LIBARCANA_INSTALL)
    set(arcana_stage_dir ${PROJECT_BINARY_DIR}/stage)
    set(arcana_staged_prefix ${arcana_stage_dir}${CMAKE_INSTALL_PREFIX})

    # Clean old stage dirs
    add_test(NAME stage_clean
             COMMAND ${CMAKE_COMMAND} -E rm -rf ${arcana_stage_dir})
    add_test(NAME stage_install
             COMMAND ${CMAKE_COMMAND} -E env DESTDIR=${arcana_stage_dir}
                     ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR})

    set_tests_properties(stage_clean PROPERTIES FIXTURES_SETUP staged)
    set_tests_properties(stage_install PROPERTIES FIXTURES_SETUP staged
                                                  DEPENDS stage_clean)

    function(arcana_add_consumer_test name)
        cmake_parse_arguments(ARG "FORCE_SCRIPT_CONFIG" "" "" ${ARGN})

        add_test(
            NAME ${name}
            COMMAND
                ${CMAKE_CTEST_COMMAND}
                --build-and-test ${CMAKE_CURRENT_SOURCE_DIR}/consumer
                                 ${CMAKE_CURRENT_BINARY_DIR}/${name}
                --build-generator ${CMAKE_GENERATOR}
                --build-makeprogram ${CMAKE_MAKE_PROGRAM}
                --build-options
                    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                    -DARCANA_PREFIX=${arcana_staged_prefix}
                    -DARCANA_DEP_PREFIX=${PROJECT_BINARY_DIR}/generators
                    -DARCANA_FORCE_SCRIPT_CONFIG=${ARG_FORCE_SCRIPT_CONFIG}
                --test-command ${CMAKE_CTEST_COMMAND})

        set_tests_properties(${name} PROPERTIES FIXTURES_REQUIRED staged)
    endfunction()

    # test both normal CMake install configs and the new CPS hotness
    arcana_add_consumer_test(consume_cps)
    arcana_add_consumer_test(consume_script FORCE_SCRIPT_CONFIG)
endif()
