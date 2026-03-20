#pragma once

#include "ParkingSpot.h"
#include <string>

namespace parking {

using namespace std;

class LargeSpot : public ParkingSpot {
public:
    explicit LargeSpot(const string& spotId) : ParkingSpot(spotId, SpotType::LARGE) {}
};

}  // namespace parking
