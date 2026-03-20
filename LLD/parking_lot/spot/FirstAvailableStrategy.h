#pragma once

#include "SpotFindingStrategy.h"
#include "../core/ParkingLot.h"
#include "../vehicle/Vehicle.h"

namespace parking {

using namespace std;

// Maps vehicle type to the spot type we try. One spot type per vehicle type.
inline SpotType spotTypeForVehicle(VehicleType vt) {
    if (vt == VehicleType::MOTORCYCLE) return SpotType::MOTORCYCLE;
    if (vt == VehicleType::CAR) return SpotType::COMPACT;
    if (vt == VehicleType::TRUCK) return SpotType::LARGE;
    return SpotType::LARGE;
}

// Simple strategy: pick the first free spot of the default type for this vehicle (e.g. CAR -> COMPACT).
class FirstAvailableStrategy : public SpotFindingStrategy {
public:
    shared_ptr<ParkingSpot> findSpot(ParkingLot& lot, VehicleType vehicleType) override {
        SpotType type = spotTypeForVehicle(vehicleType);
        return lot.getAvailableSpot(type);
    }
};

}  // namespace parking
