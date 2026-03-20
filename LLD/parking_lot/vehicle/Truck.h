#pragma once

#include "Vehicle.h"
#include <string>

namespace parking {

using namespace std;

class Truck : public Vehicle {
public:
    explicit Truck(const string& licensePlate)
        : Vehicle(licensePlate, VehicleType::TRUCK) {}
};

}  // namespace parking
