#pragma once

#include "ParkingSpot.h"
#include <string>

namespace parking {

using namespace std;

class HandicappedSpot : public ParkingSpot {
public:
    explicit HandicappedSpot(const string& spotId) : ParkingSpot(spotId, SpotType::HANDICAPPED) {}
};

}  // namespace parking
