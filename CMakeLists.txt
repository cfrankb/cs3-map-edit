cmake_minimum_required(VERSION 3.16)
project(MyQtApp LANGUAGES CXX)

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

set(HEADERS
    src/dlgabout.h
    src/dlgattr.h
    src/dlgresize.h
    src/dlgstat.h
    src/keyvaluedialog.h
    src/mapwidget.h
    src/tilebox.h
    src/dlgmsgs.h
    src/dlgselect.h
    src/dlgtest.h
    src/mainwindow.h
    src/mapscroll.h
)

set(UI_FILES
    src/dlgabout.ui
    src/dlgattr.ui
    src/dlgmsgs.ui
    src/dlgresize.ui
    src/dlgselect.ui
    src/dlgstat.ui
    src/dlgtest.ui
    src/mainwindow.ui
)


# Create executable with Qt-aware handling
qt_add_executable(MyQtApp
    MANUAL_FINALIZATION
    ${UI_FILES}
    ${CPP_SOURCES}
    ${HEADERS}
    ${RESOURCES}
)


# Add Hunspell
add_subdirectory(external/hunspell hunspell)

# Link Qt libraries
target_link_libraries(MyQtApp PRIVATE Qt6::Widgets)

#find_library(XMP_LIBRARY xmp)
find_library(ZLIB_LIBRARY z)
#find_library(OGG_LIBRARY ogg)
#find_library(VORBIS_LIBRARY vorbisfile)


# Optional: include Hunspell if available
# find_package(Hunspell REQUIRED)
target_link_libraries(MyQtApp PRIVATE hunspell z)


target_include_directories(MyQtApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/external/hunspell/src/hunspell
)

# Finalize Qt setup
qt_finalize_executable(MyQtApp)
