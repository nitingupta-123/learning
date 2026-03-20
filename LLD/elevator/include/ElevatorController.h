/**
 * @file ElevatorController.h
 * @brief Singleton orchestrator: receives requests, delegates to scheduler, manages system state.
 *
 * DESIGN NOTES:
 * 1) Singleton — One controller coordinates all elevators; double-checked locking (or
 *    std::call_once) ensures thread-safe creation.
 * 2) SRP — Controller only orchestrates: receives hall/car calls, asks scheduler for
 *    best elevator, manages NORMAL vs FIRE_ALARM and pending queue when all at capacity.
 * 3) Observer — Elevators call back on arrival (onElevatorArrival) so we can dispatch
 *    buffered pending requests.
 * 4) Concurrency — System state is std::atomic so all threads see FIRE_ALARM immediately.
 */

#ifndef ELEVATOR_ELEVATOR_CONTROLLER_H
#define ELEVATOR_ELEVATOR_CONTROLLER_H

#include "Types.h"
#include "ExternalRequest.h"
#include "Elevator.h"
#include "IScheduler.h"
#include <memory>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>

namespace elevator {

class ElevatorController {
public:
    /**
     * Thread-safe Singleton access. First call initializes with given params.
     * CONCEPT: Double-checked locking would use a static pointer + mutex; we use
     * std::call_once for clarity and correctness in C++11.
     */
    static ElevatorController& getInstance(int numElevators = 4,
                                           int numFloors = 50,
                                           int maxLoadKg = 800);

    /// Prevent copy and move.
    ElevatorController(const ElevatorController&) = delete;
    ElevatorController& operator=(const ElevatorController&) = delete;

    // --- UC1: Hall call (passenger presses UP/DOWN on a floor) ---
    void handleExternalRequest(int floor, Direction direction);

    // --- UC2: Car call (passenger selects floor inside elevator) ---
    void handleInternalRequest(int elevatorId, int destinationFloor);

    // --- UC4: Fire alarm — all elevators to ground, reject new requests ---
    void handleFireAlarm();

    /// Observer callback: elevator notifies it arrived at a floor (try to dispatch pending).
    void onElevatorArrival(int elevatorId, int floor);

    /// For simulation: tick all elevators (call move() on each).
    void tick();

    std::vector<std::unique_ptr<Elevator>>& getElevators() { return elevators_; }
    SystemState getSystemState() const { return systemState_.load(); }

private:
    ElevatorController(int numElevators, int numFloors, int maxLoadKg);

    static std::once_flag once_;
    static std::unique_ptr<ElevatorController> instance_;

    std::vector<std::unique_ptr<Elevator>> elevators_;
    std::unique_ptr<IScheduler> scheduler_;
    std::atomic<SystemState> systemState_;

    /// When all elevators are at capacity, buffer hall calls (UC5).
    std::queue<ExternalRequest> pendingQueue_;
    std::mutex pendingMutex_;
};

} // namespace elevator

#endif // ELEVATOR_ELEVATOR_CONTROLLER_H
