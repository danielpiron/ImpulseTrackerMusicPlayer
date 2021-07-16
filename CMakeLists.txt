cmake_minimum_required(VERSION 3.0.0)
project(player VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)


include(CTest)
enable_testing()

file(GLOB PLAYER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/player/*.cpp)
add_subdirectory(tests)
add_subdirectory(deps/portaudio)

add_executable(player ${PLAYER_SOURCE} src/paex_sine_c++.cpp)
target_link_libraries(player PUBLIC portaudio)
target_include_directories(player PRIVATE "${PROJECT_BINARY_DIR}" ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(player PUBLIC "${PROJECT_BINARY_DIR}" ${CMAKE_CURRENT_SOURCE_DIR}/vendors/portaudio/include)