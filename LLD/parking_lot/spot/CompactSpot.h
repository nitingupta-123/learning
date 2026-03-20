#pragma once

#include "ParkingSpot.h"
#include <string>

namespace parking {

using namespace std;

// "class CompactSpot : public ParkingSpot" = CompactSpot inherits from ParkingSpot.
// "public" means we keep ParkingSpot's public methods as public in CompactSpot.
class CompactSpot : public ParkingSpot {
public:
    // explicit = only use this constructor when we clearly mean to create a CompactSpot.
    // Stops the compiler from doing hidden conversions (e.g. from string to CompactSpot).
    explicit CompactSpot(const string& spotId)
        // Call base class constructor first. Must be here in the initializer list, not in body.
        : ParkingSpot(spotId, SpotType::COMPACT) {}
};

}  // namespace parking
