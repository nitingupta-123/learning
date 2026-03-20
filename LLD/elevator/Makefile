# Elevator System — build with g++ or clang++ (C++17)
CXX ?= g++
CXXFLAGS = -std=c++17 -I include -Wall -Wextra
BUILD_DIR = build
SRC = src/Elevator.cpp src/SCANScheduler.cpp src/ElevatorController.cpp src/main.cpp

$(BUILD_DIR)/elevator_demo: $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

build: $(BUILD_DIR)/elevator_demo

run: $(BUILD_DIR)/elevator_demo
	./$(BUILD_DIR)/elevator_demo

clean:
	rm -rf $(BUILD_DIR)

.PHONY: build run clean
