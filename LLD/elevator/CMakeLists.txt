# Elevator System LLD — C++ build
# Requires C++17 for std::optional and structured bindings if used.

cmake_minimum_required(VERSION 3.14)
project(elevator_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(ELEVATOR_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(ELEVATOR_SOURCES
    src/Elevator.cpp
    src/SCANScheduler.cpp
    src/ElevatorController.cpp
)

add_executable(elevator_demo src/main.cpp ${ELEVATOR_SOURCES})
target_include_directories(elevator_demo PRIVATE ${ELEVATOR_INCLUDE_DIR})

# Optional: static library for reuse
add_library(elevator_lib ${ELEVATOR_SOURCES})
target_include_directories(elevator_lib PUBLIC ${ELEVATOR_INCLUDE_DIR})
