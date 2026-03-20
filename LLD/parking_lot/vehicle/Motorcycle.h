#pragma once

#include "Vehicle.h"
#include <string>

namespace parking {

using namespace std;

class Motorcycle : public Vehicle {
public:
    explicit Motorcycle(const string& licensePlate)
        : Vehicle(licensePlate, VehicleType::MOTORCYCLE) {}
};

}  // namespace parking
