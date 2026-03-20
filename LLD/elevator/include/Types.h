/**
 * @file Types.h
 * @brief Central type definitions (enums) for the Elevator System LLD.
 *
 * DESIGN NOTE — Why separate enums file?
 * All shared state and direction types live here so Elevator, Controller,
 * and Scheduler can use them without circular includes. Single source of truth.
 */

#ifndef ELEVATOR_TYPES_H
#define ELEVATOR_TYPES_H

namespace elevator {

/**
 * Direction of travel or request.
 * - UP/DOWN: elevator moving or hall-call direction.
 * - IDLE: elevator is stationary (no current direction).
 */
enum class Direction {
    UP,
    DOWN,
    IDLE
};

/**
 * Physical state of a single elevator (State Pattern candidate).
 * Behavior of move(), addStop(), openDoors() changes per state.
 * - IDLE: no stops queued, waiting for requests.
 * - MOVING_UP / MOVING_DOWN: servicing queues in that direction.
 * - DOORS_OPEN: at a floor, loading/unloading; no movement.
 * - OUT_OF_SERVICE: elevator excluded from scheduling.
 */
enum class ElevatorState {
    IDLE,
    MOVING_UP,
    MOVING_DOWN,
    DOORS_OPEN,
    OUT_OF_SERVICE
};

/**
 * Global system state — overrides normal elevator logic when FIRE_ALARM.
 * Used with std::atomic for lock-free, visibility-guaranteed reads from all threads.
 */
enum class SystemState {
    NORMAL,
    FIRE_ALARM
};

} // namespace elevator

#endif // ELEVATOR_TYPES_H
