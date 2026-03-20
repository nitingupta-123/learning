#pragma once

#include "ParkingSpot.h"
#include <string>

namespace parking {

using namespace std;

class MotorcycleSpot : public ParkingSpot {
public:
    explicit MotorcycleSpot(const string& spotId) : ParkingSpot(spotId, SpotType::MOTORCYCLE) {}
};

}  // namespace parking
