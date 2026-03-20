# Elevator System — Logic Notes

These notes document the **logic and concepts** used in the code. Each concept is also attached as comments in the relevant source files.

---

## 1. Use Case Flow (Logic)

- **UC1 Hall call:** Passenger presses UP/DOWN on floor F → `handleExternalRequest(F, dir)` → if not FIRE_ALARM, create `ExternalRequest` → scheduler returns best elevator → `elevator->addStop(F)`. If all at capacity, push request to `pendingQueue_`.
- **UC2 Car call:** Passenger in elevator E presses floor G → `handleInternalRequest(E, G)` → `elevators[E]->addStop(G)` (no scheduler).
- **UC3 Servicing a stop:** Each tick, each elevator’s `move()` runs: advance one floor toward `getNextStop()`; if current floor == next stop, remove stop, set DOORS_OPEN, call `onElevatorArrival`. (In a real system, a timer would later call `closeDoors()`.)
- **UC4 Fire alarm:** `handleFireAlarm()` → set `systemState_ = FIRE_ALARM` → each elevator `emergencyReturnToGround()` (clear queues, add only floor 1). All new requests are rejected while state is FIRE_ALARM.
- **UC5 At capacity:** Scheduler skips elevators with `isAtCapacity()`; if no elevator is chosen, request is enqueued in `pendingQueue_`. When any elevator arrives at a floor, `onElevatorArrival` tries to assign one pending request.
- **UC6 Floor “behind”:** In `Elevator::addStop(floor)`: if moving UP and `floor < currentFloor_` → add to `downQueue_`; if moving DOWN and `floor > currentFloor_` → add to `upQueue_`. Otherwise add to the queue that matches the side of current floor (above → upQueue, below → downQueue).

---

## 2. SCAN Algorithm (Data Structures & Order)

- **upQueue:** `std::set<int>` — ascending. Next stop when going UP = `*upQueue_.begin()`.
- **downQueue:** `std::set<int, std::greater<int>>` — descending. Next stop when going DOWN = `*downQueue_.begin()` (highest requested floor below current).
- **getNextStop():**  
  - If direction is UP or IDLE: if upQueue non-empty → its first; else downQueue’s first (then we’ll go DOWN).  
  - If direction is DOWN: if downQueue non-empty → its first; else upQueue’s first.
- **Why set:** O(log n) insert, O(log n) erase, O(log n) duplicate check — avoids duplicate stops when many passengers select the same floor.

---

## 3. Scheduler Cost (SCANScheduler)

- Skip elevator if `isAtCapacity()` or `getState() == OUT_OF_SERVICE`.
- **cost:**  
  - IDLE: `|currentFloor - request.floor|`.  
  - Same direction and request ahead: `distance`.  
  - Same direction but behind: `distance + 2 * numFloors`.  
  - Opposite direction: `distance + numFloors`.  
- Assign elevator with **minimum cost**.

---

## 4. Thread Safety (Concurrency)

- **Elevator:** `queueMutex_` in `addStop()` and `move()`; `currentLoad_` and `state_`/`direction_` are atomic where read from outside.
- **Controller:** `systemState_` is atomic; `pendingQueue_` is protected by `pendingMutex_`.
- **Singleton:** `std::call_once` + static `instance_` so only one controller is created, with first-call parameters.

---

## 5. Where to Find It in Code

| Concept | File(s) |
|--------|---------|
| Enums (Direction, states) | `Types.h` |
| Hall/car value objects | `ExternalRequest.h`, `InternalRequest.h` |
| SCAN queues, addStop edge case, move() | `Elevator.h`, `Elevator.cpp` |
| Strategy (scheduler interface) | `IScheduler.h` |
| SCAN cost and assignment | `SCANScheduler.h`, `SCANScheduler.cpp` |
| Singleton, pending queue, fire alarm | `ElevatorController.h`, `ElevatorController.cpp` |

All of these are documented with inline comments and “DESIGN NOTE” blocks in the code.
