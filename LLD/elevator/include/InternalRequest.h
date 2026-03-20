/**
 * @file InternalRequest.h
 * @brief Value object for Car Call requests (passenger selects floor inside elevator).
 *
 * DESIGN NOTE — Value Object:
 * Immutable; carries only destination floor and which elevator it belongs to.
 * Controller uses elevatorId to dispatch to the correct Elevator instance.
 */

#ifndef ELEVATOR_INTERNAL_REQUEST_H
#define ELEVATOR_INTERNAL_REQUEST_H

namespace elevator {

struct InternalRequest {
    int destinationFloor;   ///< Floor button pressed inside the car.
    int elevatorId;         ///< Which elevator received this request.

    InternalRequest(int dest, int elevId)
        : destinationFloor(dest), elevatorId(elevId) {}
};

} // namespace elevator

#endif // ELEVATOR_INTERNAL_REQUEST_H
