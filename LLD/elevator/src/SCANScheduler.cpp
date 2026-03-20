/**
 * @file SCANScheduler.cpp
 * @brief SCAN cost computation: IDLE = distance; same direction ahead = distance; else penalties.
 */

#include "SCANScheduler.h"
#include "Types.h"
#include <limits>
#include <cmath>

namespace elevator {

SCANScheduler::SCANScheduler(int numFloors) : numFloors_(numFloors) {}

Elevator* SCANScheduler::assignElevator(const ExternalRequest& request,
                                        std::vector<std::unique_ptr<Elevator>>& elevators) {
    Elevator* best = nullptr;
    int minCost = std::numeric_limits<int>::max();

    for (auto& e : elevators) {
        if (e->isAtCapacity() || e->getState() == ElevatorState::OUT_OF_SERVICE)
            continue;
        int cost = computeCost(e.get(), request);
        if (cost < minCost) {
            minCost = cost;
            best = e.get();
        }
    }
    return best;
}

int SCANScheduler::computeCost(Elevator* e, const ExternalRequest& req) const {
    int cur = e->getCurrentFloor();
    int dist = std::abs(cur - req.floor);

    if (e->getState() == ElevatorState::IDLE)
        return dist;

    Direction elevDir = e->getDirection();
    bool sameDir = (elevDir == Direction::UP && req.direction == Direction::UP) ||
                   (elevDir == Direction::DOWN && req.direction == Direction::DOWN);
    bool ahead = (elevDir == Direction::UP && req.floor >= cur) ||
                 (elevDir == Direction::DOWN && req.floor <= cur);

    if (sameDir && ahead)
        return dist;
    if (sameDir && !ahead)
        return dist + 2 * numFloors_;  // full-sweep penalty
    return dist + numFloors_;          // opposite direction penalty
}

} // namespace elevator
