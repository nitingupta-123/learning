/**
 * @file main.cpp
 * @brief Demo: elevator system usage — hall calls, car calls, tick simulation, fire alarm.
 *
 * CONCEPT: Controller is a singleton; we get it once with (4 elevators, 50 floors, 800 kg).
 * We simulate time by calling controller.tick() repeatedly; each tick moves all elevators
 * one step. In production, tick() would be driven by a timer or event loop.
 */

#include "ElevatorController.h"
#include "Types.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace elevator;

int main() {
    // Singleton: 4 elevators, 50 floors, 800 kg max load per elevator
    ElevatorController& controller = ElevatorController::getInstance(4, 50, 800);

    std::cout << "=== Elevator System Demo ===\n\n";

    // UC1: Hall call — passenger on floor 12 presses UP
    std::cout < < "Hall call: floor 12 UP\n";
    controller.handleExternalRequest(12, Direction::UP);

    // UC2: Car call — inside elevator 0, passenger selects floor 35
    std::cout << "Car call: elevator 0 -> floor 35\n";
    controller.handleInternalRequest(0, 35);

    // Simulate a few ticks so elevator moves
    for (int t = 0; t < 100; ++t) {
        controller.tick();
        auto& elevators = controller.getElevators();
        if (t % 10 == 0 && t > 0) {
            std::cout << "  Tick " << t << ": Elevator 0 at floor "
                      << elevators[0]->getCurrentFloor()
                      << " state=" << static_cast<int>(elevators[0]->getState()) << "\n";
        }
    }

    // UC4: Fire alarm — all go to ground
    std::cout << "\nFire alarm triggered.\n";
    controller.handleFireAlarm();
    std::cout << "System state: FIRE_ALARM. New requests will be rejected.\n";

    // Drain a few ticks so elevators move toward floor 1
    for (int t = 0; t < 50; ++t)
        controller.tick();

    std::cout << "\nDone. Elevator 0 final floor: "
              << controller.getElevators()[0]->getCurrentFloor() << "\n";
    return 0;
}
