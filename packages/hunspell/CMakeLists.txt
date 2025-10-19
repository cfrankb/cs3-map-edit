cmake_minimum_required(VERSION 3.14)
project(hunspell_wrapper)

file(GLOB HUNSPELL_SOURCES
    src/hunspell/*.cxx
)

add_library(hunspell STATIC ${HUNSPELL_SOURCES})

target_include_directories(hunspell PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hunspell
)

target_compile_definitions(hunspell PRIVATE
    -DHUNSPELL_STATIC
)
