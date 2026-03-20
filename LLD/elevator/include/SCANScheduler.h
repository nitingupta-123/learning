/**
 * @file SCANScheduler.h
 * @brief SCAN ("Look") algorithm: minimum-cost assignment for hall calls.
 *
 * DESIGN NOTE — Scoring:
 * - IDLE elevator: cost = distance to request floor.
 * - Same direction and request "ahead": cost = distance (cheap).
 * - Same direction but request "behind": cost = full-sweep penalty (2 * numFloors).
 * - Opposite direction: moderate penalty (numFloors).
 * - At capacity or OUT_OF_SERVICE: skip (infinite cost).
 * We assign the elevator with minimum cost for responsiveness (minimize wait time).
 */

#ifndef ELEVATOR_SCANSCHEDULER_H
#define ELEVATOR_SCANSCHEDULER_H

#include "IScheduler.h"

namespace elevator {

class SCANScheduler : public IScheduler {
public:
    explicit SCANScheduler(int numFloors = 50);

    Elevator* assignElevator(const ExternalRequest& request,
                             std::vector<std::unique_ptr<Elevator>>& elevators) override;

private:
    /**
     * Cost for assigning this elevator to the request (lower = better).
     */
    int computeCost(Elevator* e, const ExternalRequest& req) const;

    int numFloors_;
};

} // namespace elevator

#endif // ELEVATOR_SCANSCHEDULER_H
