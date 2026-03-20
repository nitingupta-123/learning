# Elevator System — C++ LLD (Low-Level Design)

This folder contains a **full C++ implementation** of the elevator system with documented design notes and comments. Every major concept (patterns, concurrency, data structures) is explained in comments next to the code.

---

## Folder Structure

```
elevator/
├── include/                    # Public headers
│   ├── Types.h                 # Enums: Direction, ElevatorState, SystemState
│   ├── ExternalRequest.h       # Value object for hall calls (floor + direction)
│   ├── InternalRequest.h       # Value object for car calls (destination + elevatorId)
│   ├── Elevator.h              # Single elevator: SCAN queues, state, load
│   ├── IScheduler.h            # Strategy interface for elevator assignment
│   ├── SCANScheduler.h         # SCAN ("Look") algorithm implementation
│   └── ElevatorController.h    # Singleton controller
├── src/
│   ├── Elevator.cpp
│   ├── SCANScheduler.cpp
│   ├── ElevatorController.cpp
│   └── main.cpp                # Demo: hall call, car call, tick, fire alarm
├── CMakeLists.txt
└── README.md                   # This file + design notes
```

---

## Design Notes (Logic & Concepts)

### 1. Requirements Summary

| Item | Value |
|------|--------|
| Elevators | 4 |
| Floors | 50 |
| Max load per elevator | Configurable (e.g. 800 kg) |
| Request types | External (hall: UP/DOWN), Internal (car: floor number) |
| System states | NORMAL, FIRE_ALARM |
| Elevator states | IDLE, MOVING_UP, MOVING_DOWN, DOORS_OPEN, OUT_OF_SERVICE |

- **Functional:** Hall/car buttons, optimal dispatch, load limit, fire alarm override.
- **Non-functional:** Concurrency-safe, extensible scheduler (Strategy), starvation-free, minimize wait time.

### 2. Design Patterns Used

- **Strategy (IScheduler / SCANScheduler):** Scheduling algorithm is swappable without touching `ElevatorController` or `Elevator` (e.g. energy-efficient or zoned scheduler).
- **Singleton (ElevatorController):** One controller for the building; thread-safe via `std::call_once`.
- **Observer:** Elevators call back `onElevatorArrival()` so the controller can dispatch buffered pending requests when capacity frees up.
- **State (conceptual):** Elevator behavior (move, addStop, doors) depends on `ElevatorState`; could be refactored to explicit State classes if needed.

### 3. Algorithm: SCAN ("Look")

- Each elevator keeps two **sorted** stop queues:
  - **upQueue:** `std::set<int>` (ascending) — floors to stop at while going UP.
  - **downQueue:** `std::set<int, std::greater<int>>` (descending) — floors while going DOWN.
- **Movement:** Going UP → service `upQueue` in order; when empty, switch to `downQueue` (and direction DOWN). Vice versa for DOWN.
- **Why set instead of priority_queue?** O(log n) add/remove and **O(log n) duplicate check** — many passengers pressing the same floor don’t create duplicate stops.
- **Starvation-free:** No floor waits more than 2 full sweeps (bounded wait).

### 4. Scheduler Cost (SCANScheduler)

- **IDLE elevator:** cost = distance to request floor.
- **Same direction and request “ahead”:** cost = distance (preferred).
- **Same direction but “behind”:** cost = distance + 2×numFloors (full-sweep penalty).
- **Opposite direction:** cost = distance + numFloors.
- **At capacity or OUT_OF_SERVICE:** not assigned (effectively infinite cost).  
Assign the elevator with **minimum cost**.

### 5. Concurrency & Thread Safety

- **Elevator queues and movement:** `std::mutex` in `Elevator` guards `addStop()` and `move()` so queues and floor/direction stay consistent.
- **Load:** `std::atomic<int> currentLoad_` so capacity checks and updates are visible across threads without locking the whole elevator.
- **System state:** `std::atomic<SystemState>` so FIRE_ALARM is visible to all threads immediately.
- **Pending queue:** When all elevators are at capacity, hall calls go into a queue protected by a mutex; `onElevatorArrival` tries to assign one pending request.

### 6. Edge Case: Request “Behind” Current Direction (UC6)

- Elevator moving **UP**, passenger requests a floor **below** current → add to **downQueue** (served on the way back).
- Elevator moving **DOWN**, passenger requests a floor **above** current → add to **upQueue** (served on next upswing).

### 7. Fire Alarm (UC4)

- `handleFireAlarm()` sets system state to FIRE_ALARM and calls `emergencyReturnToGround()` on every elevator.
- Each elevator clears both queues and adds only floor 1, then moves down. New external/internal requests are rejected while state is FIRE_ALARM.

---

## Build & Run

```bash
cd elevator
mkdir build && cd build
cmake ..
make
./elevator_demo
```

---

## Where Concepts Are Documented in Code

- **Types.h** — Enums and their meaning (Direction, ElevatorState, SystemState).
- **ExternalRequest.h / InternalRequest.h** — Value objects.
- **Elevator.h** — SRP, SCAN queues, thread safety, edge case (behind current direction).
- **Elevator.cpp** — `addStop` logic for “behind” case, `getNextStop()` SCAN order.
- **IScheduler.h** — Strategy pattern and extensibility.
- **SCANScheduler.h/cpp** — Cost function and assignment rules.
- **ElevatorController.h** — Singleton, SRP, observer, atomic system state.
- **ElevatorController.cpp** — Pending queue and first-call init.

All of the above are reflected in short comments and “DESIGN NOTE” blocks next to the relevant code.
