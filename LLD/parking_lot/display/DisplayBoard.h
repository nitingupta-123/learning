#pragma once

#include "../enums/enums.h"

namespace parking {

using namespace std;

// Abstract base for display boards. Each subclass shows available spots (e.g. per floor or global).
class DisplayBoard {
public:
    virtual ~DisplayBoard() {}

    // Pure virtual: subclasses must implement this. Update the count shown for this spot type.
    virtual void update(SpotType spotType, int count) = 0;
};

}  // namespace parking
