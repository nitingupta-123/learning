/**
 * @file ExternalRequest.h
 * @brief Value object for Hall Call requests (passenger presses UP/DOWN on a floor).
 *
 * DESIGN NOTE — Value Object:
 * Immutable data carrier; no identity, only attributes. Used when
 * controller receives a hall call and passes it to the scheduler.
 */

#ifndef ELEVATOR_EXTERNAL_REQUEST_H
#define ELEVATOR_EXTERNAL_REQUEST_H

#include "Types.h"
#include <cstdint>

namespace elevator {

struct ExternalRequest {
    int floor;              ///< Floor where button was pressed (1..numFloors).
    Direction direction;    ///< UP or DOWN (passenger's desired travel direction).
    int64_t timestamp;      ///< For fairness / debugging; optional for scheduling.

    ExternalRequest(int f, Direction d, int64_t ts = 0)
        : floor(f), direction(d), timestamp(ts) {}
};

} // namespace elevator

#endif // ELEVATOR_EXTERNAL_REQUEST_H
