cmake_minimum_required(VERSION 3.16)
set(PROJECT_NAME "cs3-map-edit")
project(${PROJECT_NAME} LANGUAGES CXX)
# C++20 is required: src/runtime/tilesdata.cpp uses designated initializers
# (a C++20 feature) for its tile tables. GCC/Clang accept them under C++17 as an
# extension, but MSVC strictly requires /std:c++20, so the standard must be 20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)   # UI
set(CMAKE_AUTOMOC ON)   # MOC
set(CMAKE_AUTORCC ON)   # QRC

# Find Qt 6 packages
# The undo/redo framework (QUndoCommand, QUndoStack) lives in the QtGui module
# across all tested Qt 6 releases (verified on 6.8.3, 6.9.3 and 6.10.3 - there is
# no separate Qt6WidgetsExtras library in any of them), so request Gui explicitly
# alongside Widgets and Core.
# Network provides QLocalServer/QLocalSocket, used in main.cpp to enforce a
# single running instance of the editor.
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Network)

# zlib is used directly by the PNG decompression code (FrameSet.cpp, PngMagic.cpp).
# Link it so the symbols (uncompress, zError) resolve.
#
# When CS3_BUILD_ZLIB_FROM_SOURCE is enabled (or when running under GitHub CI,
# where the CI environment variable is set), build zlib from the bundled
# external/zlib source instead of relying on a system-installed libz.
set(CS3_ZLIB "")
if(CS3_BUILD_ZLIB_FROM_SOURCE OR DEFINED ENV{CI})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib/CMakeLists.txt")
        add_subdirectory(external/zlib zlib)
        set(CS3_ZLIB ZLIB::ZLIB)
    else()
        message(WARNING "external/zlib source not found; falling back to system zlib")
        find_library(ZLIB_LIBRARY z)
        if(ZLIB_LIBRARY)
            set(CS3_ZLIB ${ZLIB_LIBRARY})
        endif()
    endif()
else()
    # Fall back to the system zlib library.
    find_library(ZLIB_LIBRARY z)
    if(ZLIB_LIBRARY)
        set(CS3_ZLIB ${ZLIB_LIBRARY})
    endif()
endif()

# ---------------------------------------------------------------------------
# mapruntime: the engine/data layer (src/runtime). Isolated as a library so it
# can be linked into the editor and into the test executables (tests/TESTING.md).
#
# Plain GLOB: re-run cmake configure when adding new .cpp files,
# e.g. `cmake -S . -B build` or Qt Creator's "Re-run CMake Configure".
# ---------------------------------------------------------------------------
file(GLOB RUNTIME_SOURCES
    src/runtime/*.cpp
    src/runtime/shared/*.cpp
    src/runtime/shared/qtgui/*.cpp
)

# The map editor uses GDI, not SDL3. The following files are runtime-origin
# (SDL3-based) and must not be compiled:
#   - tileset_tex.cpp: runtime uses SDL3; the map editor uses GDI
list(REMOVE_ITEM RUNTIME_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/tileset_tex.cpp"
)

add_library(mapruntime STATIC ${RUNTIME_SOURCES})

# Define SDL_NOT_WANTED so the SDL headers compile to no-ops, avoiding pulling
# SDL functionality into the editor. USE_QFILE selects the QFile-backed file
# layer. Both are tested by runtime headers, so they must be PUBLIC to keep
# consumers consistent with the library itself.
target_compile_definitions(mapruntime PUBLIC SDL_NOT_WANTED=1 USE_QFILE=1)
target_include_directories(mapruntime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)
target_link_libraries(mapruntime PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets ${CS3_ZLIB})

# ---------------------------------------------------------------------------
# Hunspell (spell checking) is optional.
# Prefer a system-installed hunspell (found via a CMake config or pkg-config);
# otherwise fall back to the bundled submodule only if it has been initialized.
# ---------------------------------------------------------------------------
find_package(Hunspell QUIET)
if(NOT TARGET hunspell)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(HUNSPELL hunspell)
    endif()
endif()
if(NOT TARGET hunspell AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/hunspell/CMakeLists.txt")
    add_subdirectory(external/hunspell hunspell)
endif()

# ---------------------------------------------------------------------------
# mapedit_core: editor logic and widgets, i.e. everything in src/ except the
# entry point, so the test executables can link it without main().
# ---------------------------------------------------------------------------
file(GLOB CORE_SOURCES src/*.cpp)
list(REMOVE_ITEM CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

set(HEADERS
    src/dlgabout.h
    src/dlgattr.h
    src/dlgresize.h
    src/dlgstat.h
    src/dlgspell.h
    src/keyvaluedialog.h
    src/MapWidget.h
    src/tilebox.h
    src/dlgselect.h
    src/dlgmaps.h
    src/dlgtest.h
    src/mainwindow.h

)

set(UI_FILES
    src/dlgabout.ui
    src/dlgattr.ui
    src/dlgresize.ui
    src/dlgselect.ui
    src/dlgstat.ui
    src/dlgtest.ui
    src/mainwindow.ui
)

add_library(mapedit_core STATIC
    ${CORE_SOURCES}
    ${UI_FILES}
    ${HEADERS}
)

# Link Qt libraries
# Core must be explicit for MinGW builds.
# Gui provides the undo/redo framework (QUndoCommand, QUndoStack) for all Qt 6
# versions, and the editor uses it directly - link it explicitly.
target_include_directories(mapedit_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(mapedit_core PUBLIC
    mapruntime
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Network
)

# Optional: include Hunspell if available.
# Only define USE_HUNSPELL when the library is actually linked, so the
# `#if defined(USE_HUNSPELL)` guards in the source compile the code out
# otherwise. PUBLIC because SpellTextEdit.h / SpellHighlighter.h expose
# <hunspell.hxx> to whoever includes them.
if(TARGET hunspell)
    # Built from the bundled submodule or found via a CMake config package.
    target_link_libraries(mapedit_core PUBLIC hunspell z)
    target_include_directories(mapedit_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/hunspell/src/hunspell)
    target_compile_definitions(mapedit_core PUBLIC USE_HUNSPELL=1)
elseif(HUNSPELL_FOUND)
    # System hunspell found via pkg-config.
    target_link_libraries(mapedit_core PUBLIC ${HUNSPELL_LIBRARIES} z)
    target_include_directories(mapedit_core PUBLIC ${HUNSPELL_INCLUDE_DIRS})
    target_compile_definitions(mapedit_core PUBLIC USE_HUNSPELL=1)
endif()

# ---------------------------------------------------------------------------
# Executable: the entry point plus the Qt resources.
# The .qrc stays in the executable on purpose - resource initializers compiled
# into a static library can be dropped by the linker unless Q_INIT_RESOURCE is
# called for them.
# ---------------------------------------------------------------------------
file(GLOB RESOURCES src/*.qrc)

# Create executable with Qt-aware handling
qt_add_executable(${PROJECT_NAME}
    MANUAL_FINALIZATION
    src/main.cpp
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE mapedit_core)

# Finalize Qt setup
qt_finalize_executable(${PROJECT_NAME})

# ---------------------------------------------------------------------------
# Tests (tests/TESTING.md): Qt Test suites, one executable per execution
# environment. Tests labelled `ci` run on GitHub CI, `local` need a developer
# machine (AUTOUIC .ui checks, hunspell, main window).
# ---------------------------------------------------------------------------
option(CS3_BUILD_TESTS "Build the Qt Test suites" ON)
if(CS3_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
