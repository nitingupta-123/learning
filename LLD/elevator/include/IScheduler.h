/**
 * @file IScheduler.h
 * @brief Strategy interface for elevator assignment (Strategy Pattern).
 *
 * DESIGN NOTE — Strategy Pattern:
 * The scheduling algorithm is swappable without touching ElevatorController or Elevator.
 * Example: swap SCANScheduler for EnergyEfficientScheduler or ZonedScheduler for
 * skyscrapers (elevators 1–5 serve floors 1–20, etc.) with zero change to core logic.
 */

#ifndef ELEVATOR_ISCHEDULER_H
#define ELEVATOR_ISCHEDULER_H

#include "ExternalRequest.h"
#include "Elevator.h"
#include <vector>
#include <memory>

namespace elevator {

/**
 * Given a hall call (ExternalRequest) and the list of elevators, return the
 * best elevator to serve it, or nullptr if all are at capacity / out of service.
 */
class IScheduler {
public:
    virtual ~IScheduler() = default;
    virtual Elevator* assignElevator(const ExternalRequest& request,
                                     std::vector<std::unique_ptr<Elevator>>& elevators) = 0;
};

} // namespace elevator

#endif // ELEVATOR_ISCHEDULER_H
