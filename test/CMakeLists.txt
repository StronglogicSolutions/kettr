cmake_minimum_required(VERSION 3.6)

project(kettr_test)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(CPR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cpr/build/lib/libcpr.so")

FetchContent_MakeAvailable(googletest)

set(SOURCE_FILES
  main.cpp
  ../src/kettr.cpp
  kettr.test.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -lcurl")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} gtest ${CPR})
include_directories("../src" "../third_party" "../third_party/cpr/include" "../third_party/json/include")