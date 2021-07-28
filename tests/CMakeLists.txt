
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB TEST_PLAYER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)

add_executable(
  test_player
  ${PLAYER_SOURCE}
  ${TEST_PLAYER_SOURCE}
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
target_include_directories(test_player PUBLIC "${PROJECT_BINARY_DIR}" ${CMAKE_CURRENT_SOURCE_DIR}/../src)

target_link_libraries(
  test_player
  gtest_main
)
target_compile_options(test_player PUBLIC ${CLANG_WARNINGS})

include(GoogleTest)
gtest_discover_tests(test_player)