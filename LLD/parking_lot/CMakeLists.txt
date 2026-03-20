cmake_minimum_required(VERSION 3.10)
project(parking_lot CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(parking_demo main.cpp)

# All code is in headers; no other .cpp files to compile.
target_include_directories(parking_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
