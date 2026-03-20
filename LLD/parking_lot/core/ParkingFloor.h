#pragma once

#include <map>
// ^^^ map = key -> value. We use SpotType (key) -> vector of spots (value).
#include <memory>
#include <vector>
// ^^^ vector = dynamic array. spots_[type] is a list of shared_ptr<ParkingSpot>.
#include "../enums/enums.h"
#include "../spot/ParkingSpot.h"

namespace parking {

using namespace std;

// One floor of the parking lot. Holds many spots, grouped by type (COMPACT, LARGE, etc.).
class ParkingFloor {
public:
    explicit ParkingFloor(int floorId) : floorId_(floorId) {}

    int getFloorId() const { return floorId_; }

    // Add a spot to this floor. The floor does not own the spot; we store a shared_ptr.
    void addSpot(shared_ptr<ParkingSpot> spot) {
        SpotType t = spot->getType();
        spots_[t].push_back(spot);
    }

    // So ParkingLot can populate its free-spot priority queue when a floor is added.
    const vector<shared_ptr<ParkingSpot>>& getSpotsByType(SpotType type) const {
        static const vector<shared_ptr<ParkingSpot>> empty;
        auto it = spots_.find(type);
        if (it == spots_.end()) return empty;
        return it->second;
    }

    // Count how many spots of this type are free on this floor.
    int getFreeSpotCount(SpotType type) {
        int count = 0;
        for (size_t i = 0; i < spots_[type].size(); i++) {
            if (spots_[type][i]->getIsFree()) count++;
        }
        return count;
    }

    // Find the first free spot of the given type. Returns nullptr if none available.
    shared_ptr<ParkingSpot> getAvailableSpot(SpotType type) {
        for (size_t i = 0; i < spots_[type].size(); i++) {
            shared_ptr<ParkingSpot> s = spots_[type][i];
            if (s->getIsFree()) return s;
        }
        return nullptr;
    }

    // True if every spot on this floor is occupied.
    bool isFull() {
        for (auto it = spots_.begin(); it != spots_.end(); it++) {
            for (size_t i = 0; i < it->second.size(); i++) {
                if (it->second[i]->getIsFree()) return false;
            }
        }
        return true;
    }

private:
    int floorId_;
    // map<Key, Value>: for each SpotType we store a vector (list) of spots. e.g. spots_[COMPACT] = [C-1, C-2].
    map<SpotType, vector<shared_ptr<ParkingSpot>>> spots_;
};

}  // namespace parking
