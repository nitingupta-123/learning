#pragma once
// ^^^ Include this file only once per .cpp - prevents "redefinition" errors.

#include <memory>
// ^^^ For shared_ptr - a smart pointer that automatically frees memory when no one uses it.
#include <string>
#include "../enums/enums.h"
#include "../vehicle/Vehicle.h"

namespace parking {

using namespace std;

// Base class for all parking spots. Only concrete spot types (CompactSpot, etc.) are created.
class ParkingSpot {
public:
    // virtual destructor: when we delete through ParkingSpot*, the correct derived destructor runs.
    virtual ~ParkingSpot() {}

    string getSpotId() const { return spotId_; }
    bool getIsFree() const { return isFree_; }
    SpotType getType() const { return type_; }
    // shared_ptr<Vehicle> = "pointer to Vehicle that is shared; memory freed when last shared_ptr is gone".
    shared_ptr<Vehicle> getAssignedVehicle() const { return assignedVehicle_; }

    void assignVehicle(shared_ptr<Vehicle> vehicle) {
        assignedVehicle_ = vehicle;
        isFree_ = false;
    }
    void removeVehicle() {
        assignedVehicle_ = nullptr;
        // ^^^ nullptr = "no object" (modern C++; older code used NULL or 0).
        isFree_ = true;
    }

protected:
    // Constructor: ":" starts the initializer list - we set members before the body runs.
    // We must call the base constructor here; we cannot call it inside { }.
    ParkingSpot(const string& id, SpotType spotType)
        : spotId_(id), isFree_(true), type_(spotType), assignedVehicle_(nullptr) {}

    string spotId_;
    bool isFree_;
    SpotType type_;
    shared_ptr<Vehicle> assignedVehicle_;
};

}  // namespace parking
