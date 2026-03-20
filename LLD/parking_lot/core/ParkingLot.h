#pragma once

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include "../enums/enums.h"
#include "../display/DisplayBoard.h"
#include "ParkingFloor.h"

namespace parking {

// Forward declarations (gates) to avoid circular include
class EntranceGate;
class ExitGate;

using namespace std;

// Min-heap by spotId so we get "nearest" (smallest id) free spot in O(log N). Compare: higher spotId = lower priority.
struct SpotIdMinCompare {
    bool operator()(const shared_ptr<ParkingSpot>& a, const shared_ptr<ParkingSpot>& b) const {
        return a->getSpotId() > b->getSpotId();
    }
};

using FreeSpotQueue = priority_queue<shared_ptr<ParkingSpot>, vector<shared_ptr<ParkingSpot>>, SpotIdMinCompare>;

// Singleton: only one ParkingLot exists in the whole program. getInstance() returns that one instance.
class ParkingLot {
public:
    // static = belongs to the class, not to each object. So getInstance() is called as ParkingLot::getInstance().
    // "static ParkingLot instance" inside the function = created once, lives until program ends.
    static ParkingLot& getInstance() {
        static ParkingLot instance;
        return instance;
    }

    void setName(const string& name) { name_ = name; }
    string getName() const { return name_; }

    void addFloor(shared_ptr<ParkingFloor> floor) {
        floors_.push_back(floor);
        SpotType types[] = {SpotType::MOTORCYCLE, SpotType::COMPACT, SpotType::LARGE, SpotType::HANDICAPPED};
        for (int t = 0; t < 4; t++) {
            const vector<shared_ptr<ParkingSpot> >& spots = floor->getSpotsByType(types[t]);
            for (size_t i = 0; i < spots.size(); i++) freeSpots_[types[t]].push(spots[i]);
        }
    }
    void addEntranceGate(shared_ptr<EntranceGate> gate) { entrances_.push_back(gate); }
    void addExitGate(shared_ptr<ExitGate> gate) { exits_.push_back(gate); }

    // O(log N): pop best free spot from priority queue (min by spotId = "nearest"). Returns nullptr if none.
    shared_ptr<ParkingSpot> getAvailableSpot(SpotType type) {
        while (!freeSpots_[type].empty()) {
            shared_ptr<ParkingSpot> spot = freeSpots_[type].top();
            freeSpots_[type].pop();
            if (spot->getIsFree()) return spot;
        }
        return nullptr;
    }

    // Call when a vehicle exits: put the spot back in the free pool so it can be reused. O(log N).
    void returnSpotToPool(shared_ptr<ParkingSpot> spot) {
        if (spot && spot->getIsFree()) freeSpots_[spot->getType()].push(spot);
    }

    bool isFull() {
        for (size_t i = 0; i < floors_.size(); i++) {
            if (!floors_[i]->isFull()) return false;
        }
        return true;
    }

    // Observer pattern: display boards register here and get notified when spot availability changes.
    // We use raw pointer so we can register singletons (e.g. GlobalDisplayBoard) without ownership.
    void addObserver(DisplayBoard* observer) { observers_.push_back(observer); }

    // Call this when a vehicle enters or exits so all display boards can update (e.g. free spot counts).
    void notifySpotChanged() {
        SpotType types[] = {SpotType::MOTORCYCLE, SpotType::COMPACT, SpotType::LARGE, SpotType::HANDICAPPED};
        for (int t = 0; t < 4; t++) {
            SpotType type = types[t];
            int count = 0;
            for (size_t i = 0; i < floors_.size(); i++) count += floors_[i]->getFreeSpotCount(type);
            for (size_t i = 0; i < observers_.size(); i++) observers_[i]->update(type, count);
        }
    }

    // = delete means "this function is forbidden". So no one can copy the singleton (e.g. ParkingLot b = getInstance();).
    ParkingLot(const ParkingLot&) = delete;
    ParkingLot& operator=(const ParkingLot&) = delete;

private:
    ParkingLot() {}
    // ^^^ Private constructor: only getInstance() can create the one instance.

    string name_;
    vector<shared_ptr<ParkingFloor>> floors_;
    vector<shared_ptr<EntranceGate>> entrances_;
    vector<shared_ptr<ExitGate>> exits_;
    vector<DisplayBoard*> observers_;
    // Free spots: Map<SpotType, PriorityQueue> for O(log N) get-best and O(log N) return. No separate
    // OccupiedSpace map: we get the spot from the ticket (ticket->getSpot()) in O(1) when a car exits.
    map<SpotType, FreeSpotQueue> freeSpots_;
};

}  // namespace parking
