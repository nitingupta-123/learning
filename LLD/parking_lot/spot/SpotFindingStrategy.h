#pragma once

#include <memory>
#include "../enums/enums.h"

namespace parking {

class ParkingLot;
class ParkingSpot;

using namespace std;

// Strategy pattern: different ways to choose an available spot (e.g. first available, nearest to entrance).
// Lets us change spot-selection logic without changing the gate code.
class SpotFindingStrategy {
public:
    virtual ~SpotFindingStrategy() {}

    // Return a free spot for this vehicle type, or nullptr if none.
    virtual shared_ptr<ParkingSpot> findSpot(ParkingLot& lot, VehicleType vehicleType) = 0;
};

}  // namespace parking
