#pragma once
// Include this file only once per .cpp.

#include <memory>
#include <string>
#include "../enums/enums.h"
#include "../spot/ParkingSpot.h"
#include "../vehicle/Vehicle.h"

namespace parking {

using namespace std;

// A ticket is issued when a vehicle enters. It links the vehicle, the spot, and entry/exit times.
class Ticket {
public:
    // Constructor: ":" starts initializer list - set all members before body. exitTime_ = 0 until exit.
    Ticket(const string& id, long entryTime, shared_ptr<ParkingSpot> spot, shared_ptr<Vehicle> vehicle)
        : ticketId_(id), entryTime_(entryTime), exitTime_(0), spot_(spot), vehicle_(vehicle),
          status_(TicketStatus::ACTIVE) {}

    string getTicketId() const { return ticketId_; }
    long getEntryTime() const { return entryTime_; }
    long getExitTime() const { return exitTime_; }
    shared_ptr<ParkingSpot> getSpot() const { return spot_; }
    shared_ptr<Vehicle> getVehicle() const { return vehicle_; }
    TicketStatus getStatus() const { return status_; }

    void setExitTime(long t) { exitTime_ = t; }
    void setStatus(TicketStatus s) { status_ = s; }

private:
    string ticketId_;
    long entryTime_;
    long exitTime_;
    shared_ptr<ParkingSpot> spot_;
    shared_ptr<Vehicle> vehicle_;
    TicketStatus status_;
};

}  // namespace parking
