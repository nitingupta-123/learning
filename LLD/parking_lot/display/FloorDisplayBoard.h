#pragma once

#include <map>
#include "DisplayBoard.h"

namespace parking {

using namespace std;

// One board per floor. Shows free spots per SpotType on this floor only.
class FloorDisplayBoard : public DisplayBoard {
public:
    explicit FloorDisplayBoard(int floorId) : floorId_(floorId) {}

    void update(SpotType spotType, int count) override {
        counts_[spotType] = count;
    }

    int getFloorId() const { return floorId_; }
    int getCount(SpotType spotType) const {
        auto it = counts_.find(spotType);
        if (it == counts_.end()) return 0;
        return it->second;
    }

private:
    int floorId_;
    map<SpotType, int> counts_;
};

}  // namespace parking
