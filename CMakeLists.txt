cmake_minimum_required(VERSION 3.0.0)
project(player VERSION 0.1.0)

# Warnings flags borrowed from Jason Turner's cpp_starter_project repo
# https://github.com/lefticus/cpp_starter_project/blob/master/cmake/CompilerWarnings.cmake
#
set(CLANG_WARNINGS
    -Wall
    -Wextra # reasonable and standard
    -Wshadow # warn the user if a variable declaration shadows one from a parent context
    -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps
                    # catch hard to track down memory errors
    # -Wold-style-cast # warn for c-style casts
    -Wcast-align # warn for potential performance problem casts
    -Wunused # warn on anything being unused
    -Woverloaded-virtual # warn if you overload (not override) a virtual function
    -Wpedantic # warn if non-standard C++ is used
    -Wconversion # warn on type conversions that may lose data
    -Wsign-conversion # warn on sign conversions
    -Wnull-dereference # warn if a null dereference is detected
    -Wdouble-promotion # warn if float is implicit promoted to double
    -Wformat=2 # warn on security issues around functions that format output (ie printf)
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(CTest)
enable_testing()

file(GLOB PLAYER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/player/*.cpp)
add_subdirectory(tests)
add_subdirectory(deps/portaudio)

add_executable(player ${PLAYER_SOURCE} src/paex_sine_c++.cpp)
target_compile_options(player PUBLIC ${CLANG_WARNINGS} -Werror -fsanitize=address)
target_link_options(player PUBLIC -fsanitize=address)
target_link_libraries(player PUBLIC portaudio)
target_include_directories(player PRIVATE "${PROJECT_BINARY_DIR}" ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(player SYSTEM PUBLIC "${PROJECT_BINARY_DIR}" ${CMAKE_CURRENT_SOURCE_DIR}/VENDORS/PORTAUDIO/INCLUDE)