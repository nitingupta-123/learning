#pragma once

#include "Vehicle.h"
#include <string>

namespace parking {

using namespace std;

class Car : public Vehicle {
public:
    explicit Car(const string& licensePlate)
        : Vehicle(licensePlate, VehicleType::CAR) {}
};

}  // namespace parking
