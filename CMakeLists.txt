cmake_minimum_required(VERSION 3.6)
project(kettr)
include_directories(src third_party third_party/cpr/include third_party/json/include third_party/kutils/include)

set(SOURCE_FILES
  src/main.cpp
  src/kettr.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -lcurl")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set(CPR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpr/build/lib/libcpr.so")
target_link_libraries(${PROJECT_NAME} ${CPR})

add_subdirectory(test)