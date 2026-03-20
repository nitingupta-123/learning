#pragma once

#include "Vehicle.h"
#include <string>

namespace parking {

using namespace std;

class Van : public Vehicle {
public:
    explicit Van(const string& licensePlate)
        : Vehicle(licensePlate, VehicleType::VAN) {}
};

}  // namespace parking
