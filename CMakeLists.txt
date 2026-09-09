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

# Emit debug info (symbols) so crashes can be backtraced.
# Always enabled; the cost is larger binaries, which is worth it for
# being able to attach a debugger / produce a usable call stack.
#if(NOT CMAKE_CXX_FLAGS)
#    set(CMAKE_CXX_FLAGS "-g")
#else()
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
#endif()



# Find Qt 6 packages
# The undo/redo framework (QUndoCommand, QUndoStack) lives in the QtGui module
# across all tested Qt 6 releases (verified on 6.8.3, 6.9.3 and 6.10.3 - there is
# no separate Qt6WidgetsExtras library in any of them), so request Gui explicitly
# alongside Widgets and Core.
# Network provides QLocalServer/QLocalSocket, used in main.cpp to enforce a
# single running instance of the editor.
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Network)

#file(GLOB HEADERS src/*.h)
file(GLOB RESOURCES src/*.qrc)

# Collect .cpp files
file(GLOB CPP_SOURCES
    src/*.cpp
    src/runtime/*.cpp
    src/runtime/shared/*.cpp
    src/runtime/shared/qtgui/*.cpp
)

# The map editor uses GDI, not SDL3. The following files are runtime-origin
# (SDL3-based) and must not be compiled into the map editor:
#   - layerdata.cpp: map editor defines its own g_layerdata in src/layerdata.cpp
#   - tileset_tex.cpp: runtime uses SDL3; the map editor uses GDI
list(REMOVE_ITEM CPP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/layerdata.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/tileset_tex.cpp"
)

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


# Create executable with Qt-aware handling
qt_add_executable(${PROJECT_NAME}
    MANUAL_FINALIZATION
    ${UI_FILES}
    ${CPP_SOURCES}
    ${HEADERS}
    ${RESOURCES}
)


# Hunspell (spell checking) is optional.
# Prefer a system-installed hunspell (found via a CMake config or pkg-config);
# otherwise fall back to the bundled submodule only if it has been initialized.
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

# Link Qt libraries
# Core must be explicit for MinGW builds.
# Gui provides the undo/redo framework (QUndoCommand, QUndoStack) for all Qt 6
# versions, and the editor uses it directly - link it explicitly.
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network)

# The map editor uses GDI, not SDL. Define SDL_NOT_WANTED so the SDL headers
# compile to no-ops, avoiding pulling SDL functionality into the editor.
target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_NOT_WANTED=1)
target_compile_definitions(${PROJECT_NAME} PRIVATE USE_QFILE=1)


# zlib is used directly by the PNG decompression code (FrameSet.cpp, PngMagic.cpp).
# Link it so the symbols (uncompress, zError) resolve.
#
# When CS3_BUILD_ZLIB_FROM_SOURCE is enabled (or when running under GitHub CI,
# where the CI environment variable is set), build zlib from the bundled
# external/zlib source instead of relying on a system-installed libz.
if(CS3_BUILD_ZLIB_FROM_SOURCE OR DEFINED ENV{CI})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib/CMakeLists.txt")
        add_subdirectory(external/zlib zlib)
        target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
    else()
        message(WARNING "external/zlib source not found; falling back to system zlib")
        find_library(ZLIB_LIBRARY z)
        if(ZLIB_LIBRARY)
            target_link_libraries(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARY})
        endif()
    endif()
else()
    # Fall back to the system zlib library.
    find_library(ZLIB_LIBRARY z)
    if(ZLIB_LIBRARY)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARY})
    endif()
endif()
#find_library(OGG_LIBRARY ogg)
#find_library(VORBIS_LIBRARY vorbisfile)


# Optional: include Hunspell if available
# find_package(Hunspell REQUIRED)
# Only define USE_HUNSPELL when the library is actually linked, so the
# `#if defined(USE_HUNSPELL)` guards in the source compile the code out
# otherwise.
if(TARGET hunspell)
    # Built from the bundled submodule or found via a CMake config package.
    target_link_libraries(${PROJECT_NAME} PRIVATE hunspell z)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/hunspell/src/hunspell)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_HUNSPELL=1)
elseif(HUNSPELL_FOUND)
    # System hunspell found via pkg-config.
    target_link_libraries(${PROJECT_NAME} PRIVATE ${HUNSPELL_LIBRARIES} z)
    target_include_directories(${PROJECT_NAME} PRIVATE ${HUNSPELL_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_HUNSPELL=1)
endif()


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/shared
)

# Finalize Qt setup
qt_finalize_executable(${PROJECT_NAME})
