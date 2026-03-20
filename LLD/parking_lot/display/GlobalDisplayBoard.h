#pragma once

#include <map>
#include "DisplayBoard.h"

namespace parking {

using namespace std;

// Singleton: one global board for the whole lot. Shows free spots per SpotType across all floors.
class GlobalDisplayBoard : public DisplayBoard {
public:
    static GlobalDisplayBoard& getInstance() {
        static GlobalDisplayBoard instance;
        return instance;
    }

    // override = we implement the abstract update() from DisplayBoard.
    void update(SpotType spotType, int count) override {
        counts_[spotType] = count;
    }

    int getCount(SpotType spotType) const {
        auto it = counts_.find(spotType);
        if (it == counts_.end()) return 0;
        return it->second;
    }

    GlobalDisplayBoard(const GlobalDisplayBoard&) = delete;
    GlobalDisplayBoard& operator=(const GlobalDisplayBoard&) = delete;

private:
    GlobalDisplayBoard() {}

    map<SpotType, int> counts_;
};

}  // namespace parking
