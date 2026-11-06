# Qt Test suites for the map editor (tests/TESTING.md).
#
# One executable per execution environment, not per source file: the split that
# matters is headless (tst_core) vs offscreen GUI (tst_gui) vs local-only
# (tst_local). Per-file granularity is kept inside them as Qt Test classes.

find_package(Qt6 REQUIRED COMPONENTS Test)

# cs3_add_test(NAME <target> SOURCES <files...> [QPA <platform>] LABELS <labels...>)
# Every suite links the editor library (which brings the runtime library with it).
function(cs3_add_test)
    cmake_parse_arguments(ARG "" "NAME;QPA" "SOURCES;LABELS" ${ARGN})
    add_executable(${ARG_NAME} ${ARG_SOURCES})
    target_link_libraries(${ARG_NAME} PRIVATE Qt6::Test mapedit_core)
    target_compile_definitions(${ARG_NAME} PRIVATE
        CS3_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        CS3_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/in"
    )
    add_test(NAME ${ARG_NAME} COMMAND ${ARG_NAME})
    set_tests_properties(${ARG_NAME} PROPERTIES LABELS "${ARG_LABELS}")
    if(ARG_QPA)
        set_tests_properties(${ARG_NAME} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=${ARG_QPA}")
    endif()
endfunction()

# Source hash tracker: fails when an inventoried source changed without its
# tests being updated, or when a new source is not tracked yet.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME tracker
        COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/scripts/track_sources.py --check
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set_tests_properties(tracker PROPERTIES LABELS ci)
else()
    message(STATUS "Python3 interpreter not found: the tracker test is skipped")
endif()

# Fixtures (tests/in) are read-only; suites that write do so under QTemporaryDir.
cs3_add_test(NAME tst_core LABELS ci SOURCES unit/tst_core.cpp)

cs3_add_test(NAME tst_gui LABELS ci QPA offscreen SOURCES unit/tst_gui.cpp)

# Needs the Qt resources (icons, tiles.obl) compiled in, because the editor
# library itself does not carry the .qrc.
cs3_add_test(NAME tst_local LABELS local QPA offscreen
    SOURCES unit/tst_local.cpp ${CMAKE_SOURCE_DIR}/src/mapedit.qrc)
