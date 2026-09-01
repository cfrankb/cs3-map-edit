cmake_minimum_required(VERSION 3.16)
set(PROJECT_NAME "cs3-map-edit")
project(${PROJECT_NAME} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)   # UI
set(CMAKE_AUTOMOC ON)   # MOC
set(CMAKE_AUTORCC ON)   # QRC

# Find Qt 6 packages
find_package(Qt6 REQUIRED COMPONENTS Widgets)

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
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

# SDL2 is used by the tileset texture code.
#find_package(SDL2 REQUIRED COMPONENTS)
#target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)

#find_library(XMP_LIBRARY xmp)
find_library(ZLIB_LIBRARY z)
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
