/**
 * @file Elevator.h
 * @brief Single elevator: state machine, SCAN queues, load, and door logic.
 *
 * DESIGN NOTES:
 * 1) SRP — Elevator is responsible only for its own physical state: movement,
 *    door operation, internal stop queues, and load tracking.
 * 2) SCAN Algorithm — Two sorted sets: upQueue (ascending), downQueue (descending).
 *    No starvation: every floor is serviced within at most 2 full sweeps.
 * 3) Thread Safety — std::mutex guards queue and state updates; std::atomic for load
 *    so capacity check is visible across threads without locking the whole elevator.
 * 4) Edge Case — If elevator is moving UP and passenger requests a floor already
 *    passed, we add to downQueue (served on the way back); similarly for DOWN.
 */

#ifndef ELEVATOR_ELEVATOR_H
#define ELEVATOR_ELEVATOR_H

#include "Types.h"
#include <set>
#include <mutex>
#include <atomic>
#include <functional>
#include <optional>

namespace elevator {

// Forward declaration to avoid circular include (Controller notifies on arrival).
class ElevatorController;

class Elevator {
public:
    using ArrivalCallback = std::function<void(int elevatorId, int floor)>;

    /**
     * @param id Unique elevator index (0..numElevators-1).
     * @param maxLoadKg Max weight in kg (e.g. 800 kg ~ 10 people).
     * @param numFloors Total floors (e.g. 50) for SCAN bounds.
     * @param onArrival Called when elevator reaches a stop (Observer pattern).
     */
    Elevator(int id, int maxLoadKg, int numFloors, ArrivalCallback onArrival);

    // --- Queue management (called by Controller / internal panel) ---

    /**
     * Add a stop (hall call or car call). Thread-safe.
     * CONCEPT: If we're moving UP and floor is below currentFloor -> add to downQueue
     * so we serve it on the way back; same for moving DOWN and floor above.
     */
    void addStop(int floor);

    /**
     * Remove a stop after servicing (internal use).
     */
    void removeStop(int floor);

    // --- Movement & doors (called by simulation tick or internal loop) ---

    /**
     * One "tick" of movement: advance one floor toward next stop, or open doors if arrived.
     * CONCEPT: getNextStop() picks from upQueue or downQueue based on current direction.
     */
    void move();

    /**
     * Set state to DOORS_OPEN; in a real system would start a timer then closeDoors().
     */
    void openDoors();

    /**
     * Set state back to MOVING_UP/MOVING_DOWN so move() can run again.
     */
    void closeDoors();

    // --- Capacity & state (read by Scheduler; thread-safe) ---

    bool isAtCapacity() const;
    int getCurrentFloor() const;
    Direction getDirection() const;
    ElevatorState getState() const;
    int getId() const;

    /**
     * Update load (e.g. from load sensor). Returns true if within capacity.
     * CONCEPT: Atomic update so scheduler's isAtCapacity() sees consistent value.
     */
    bool updateLoad(int deltaKg);

    // --- Fire alarm (system override) ---

    /**
     * Clear all queues and add only floor 1; elevator goes to ground.
     */
    void emergencyReturnToGround();

private:
    /**
     * Next floor to stop at based on current direction and queues (SCAN).
     * Returns nullopt when both queues are empty.
     */
    std::optional<int> getNextStop();

    const int id_;
    const int maxLoad_;
    const int numFloors_;
    ArrivalCallback onArrival_;

    /// Current position and direction/state — read under lock or kept consistent.
    int currentFloor_;
    std::atomic<ElevatorState> state_;
    std::atomic<Direction> direction_;

    /**
     * SCAN queues: TreeSet equivalent in C++.
     * - upQueue: ascending order (default std::set<int>).
     * - downQueue: descending via std::greater — we take "first" as highest floor.
     */
    std::set<int> upQueue_;
    std::set<int, std::greater<int>> downQueue_;

    std::atomic<int> currentLoad_;
    mutable std::mutex queueMutex_;  ///< Protects queues + currentFloor/direction for addStop/move.
};

} // namespace elevator

#endif // ELEVATOR_ELEVATOR_H
