/**
 * @file ElevatorController.cpp
 * @brief Singleton creation, request handling, fire alarm, pending queue dispatch.
 */

#include "ElevatorController.h"
#include "SCANScheduler.h"
#include <algorithm>

namespace elevator {

std::once_flag ElevatorController::once_;
std::unique_ptr<ElevatorController> ElevatorController::instance_;

ElevatorController& ElevatorController::getInstance(int numElevators,
                                                    int numFloors,
                                                    int maxLoadKg) {
    // Capture by value so first call's parameters are used for construction.
    std::call_once(once_, [numElevators, numFloors, maxLoadKg]() {
        instance_ = std::unique_ptr<ElevatorController>(
            new ElevatorController(numElevators, numFloors, maxLoadKg));
    });
    return *instance_;
}

ElevatorController::ElevatorController(int numElevators, int numFloors, int maxLoadKg)
    : systemState_(SystemState::NORMAL)
{
    scheduler_ = std::make_unique<SCANScheduler>(numFloors);

    // Observer: when an elevator arrives at a floor, notify controller to try pending.
    auto onArrival = [this](int elevatorId, int floor) {
        this->onElevatorArrival(elevatorId, floor);
    };

    elevators_.reserve(static_cast<size_t>(numElevators));
    for (int i = 0; i < numElevators; ++i)
        elevators_.push_back(std::make_unique<Elevator>(i, maxLoadKg, numFloors, onArrival));
}

void ElevatorController::handleExternalRequest(int floor, Direction direction) {
    if (systemState_.load() == SystemState::FIRE_ALARM)
        return;

    ExternalRequest request(floor, direction);
    Elevator* best = scheduler_->assignElevator(request, elevators_);

    if (best) {
        best->addStop(floor);
    } else {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingQueue_.push(request);
    }
}

void ElevatorController::handleInternalRequest(int elevatorId, int destinationFloor) {
    if (systemState_.load() == SystemState::FIRE_ALARM)
        return;
    if (elevatorId < 0 || static_cast<size_t>(elevatorId) >= elevators_.size())
        return;
    elevators_[static_cast<size_t>(elevatorId)]->addStop(destinationFloor);
}

void ElevatorController::onElevatorArrival(int elevatorId, int floor) {
    (void)elevatorId;
    (void)floor;
    // Try to dispatch one buffered pending request (UC5: when elevators free up).
    std::lock_guard<std::mutex> lock(pendingMutex_);
    if (pendingQueue_.empty()) return;

    ExternalRequest next = pendingQueue_.front();
    Elevator* best = scheduler_->assignElevator(next, elevators_);
    if (best) {
        pendingQueue_.pop();
        best->addStop(next.floor);
    }
}

void ElevatorController::handleFireAlarm() {
    systemState_.store(SystemState::FIRE_ALARM);
    for (auto& e : elevators_)
        e->emergencyReturnToGround();
}

void ElevatorController::tick() {
    for (auto& e : elevators_)
        e->move();
}

} // namespace elevator
