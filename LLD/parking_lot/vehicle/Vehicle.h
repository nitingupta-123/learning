#pragma once
// ^^^ "Include this file only once" - avoids errors when many files include this header.

#include <string>
#include "../enums/enums.h"

namespace parking {
// ^^^ Puts our names (Vehicle, Car, etc.) inside "parking" so they don't clash with other code.

using namespace std;
// ^^^ So we can write "string" instead of "std::string" in this file.

// Base class for all vehicles. We never create "Vehicle" directly,
// only Car, Truck, etc. So the constructor is protected.
class Vehicle {
public:
    // virtual = "call the right destructor at runtime" when we delete through a base pointer.
    // e.g. Vehicle* v = new Car(...); delete v;  -> ~Car() runs, then ~Vehicle().
    virtual ~Vehicle() {}

    // "const" at the end = this function does not change the object (read-only).
    string getLicensePlate() const { return licensePlate_; }
    VehicleType getType() const { return type_; }

protected:
// ^^^ Only this class and its subclasses (Car, Truck, ...) can see what's below. Not the outside world.

    // Only subclasses (Car, Truck, ...) can call this. So nobody can do Vehicle v(...); directly.
    // const string& = "pass the string by reference, don't copy it". & means reference.
    Vehicle(const string& plate, VehicleType vehicleType) {
        licensePlate_ = plate;
        type_ = vehicleType;
    }

    // Trailing _ is a style: "this is a member variable of the class".
    string licensePlate_;
    VehicleType type_;
};

}  // namespace parking
