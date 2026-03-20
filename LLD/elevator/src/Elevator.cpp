/**
 * @file Elevator.cpp
 * @brief Implementation of Elevator: SCAN queues, addStop edge cases, move(), emergency.
 */

#include "Elevator.h"
#include <algorithm>

namespace elevator {

Elevator::Elevator(int id, int maxLoadKg, int numFloors, ArrivalCallback onArrival)
    : id_(id)
    , maxLoad_(maxLoadKg)
    , numFloors_(numFloors)
    , onArrival_(std::move(onArrival))
    , currentFloor_(1)
    , state_(ElevatorState::IDLE)
    , currentLoad_(0)
{
    direction_.store(Direction::IDLE);
}

void Elevator::addStop(int floor) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    // Bounds check: floor must be in [1, numFloors_] (uses numFloors_ to document invariant).
    if (floor < 1 || floor > numFloors_) return;
    ElevatorState s = state_.load();

    // EDGE CASE (UC6): Elevator moving UP but passenger wants a floor we already passed.
    if (s == ElevatorState::MOVING_UP && floor < currentFloor_)
        downQueue_.insert(floor);  // Serve on the way back (downQueue is descending).
    else if (s == ElevatorState::MOVING_DOWN && floor > currentFloor_)
        upQueue_.insert(floor);   // Serve on next upswing.
    else if (floor >= currentFloor_)
        upQueue_.insert(floor);
    else
        downQueue_.insert(floor);
}

void Elevator::removeStop(int floor) {
    upQueue_.erase(floor);
    downQueue_.erase(floor);
}

std::optional<int> Elevator::getNextStop() {
    Direction d = direction_.load();
    if (d == Direction::UP || d == Direction::IDLE) {
        if (!upQueue_.empty())   return *upQueue_.begin();
        if (!downQueue_.empty()) return *downQueue_.begin();  // highest in downQueue
    } else {
        if (!downQueue_.empty()) return *downQueue_.begin();
        if (!upQueue_.empty())   return *upQueue_.begin();
    }
    return std::nullopt;
}

void Elevator::move() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (state_.load() == ElevatorState::DOORS_OPEN ||
        state_.load() == ElevatorState::OUT_OF_SERVICE)
        return;

    auto next = getNextStop();
    if (!next) {
        state_.store(ElevatorState::IDLE);
        direction_.store(Direction::IDLE);
        return;
    }

    int nextFloor = *next;
    if (nextFloor > currentFloor_)
        direction_.store(Direction::UP);
    else
        direction_.store(Direction::DOWN);

    state_.store(direction_.load() == Direction::UP
                     ? ElevatorState::MOVING_UP
                     : ElevatorState::MOVING_DOWN);

    currentFloor_ += (direction_.load() == Direction::UP ? 1 : -1);

    if (currentFloor_ == nextFloor) {
        removeStop(nextFloor);
        state_.store(ElevatorState::DOORS_OPEN);
        if (onArrival_)
            onArrival_(id_, currentFloor_);
    }
}

void Elevator::openDoors() {
    state_.store(ElevatorState::DOORS_OPEN);
}

void Elevator::closeDoors() {
    Direction d = direction_.load();
    state_.store(d == Direction::UP ? ElevatorState::MOVING_UP : ElevatorState::MOVING_DOWN);
}

bool Elevator::isAtCapacity() const {
    return currentLoad_.load() >= maxLoad_;
}

int Elevator::getCurrentFloor() const {
    return currentFloor_;
}

Direction Elevator::getDirection() const {
    return direction_.load();
}

ElevatorState Elevator::getState() const {
    return state_.load();
}

int Elevator::getId() const {
    return id_;
}

bool Elevator::updateLoad(int deltaKg) {
    int prev = currentLoad_.load();
    int desired = prev + deltaKg;
    if (desired > maxLoad_) return false;
    while (!currentLoad_.compare_exchange_weak(prev, desired))
        desired = prev + deltaKg;
    return true;
}

void Elevator::emergencyReturnToGround() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    upQueue_.clear();
    downQueue_.clear();
    downQueue_.insert(1);
    direction_.store(Direction::DOWN);
    state_.store(ElevatorState::MOVING_DOWN);
}

} // namespace elevator
