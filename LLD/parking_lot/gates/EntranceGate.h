#pragma once

#include <ctime>
#include <memory>
#include <string>
#include "../core/ParkingLot.h"
#include "../core/Ticket.h"
#include "../enums/enums.h"
#include "../spot/FirstAvailableStrategy.h"
#include "../spot/ParkingSpot.h"
#include "../vehicle/Vehicle.h"

namespace parking {

using namespace std;

class EntranceGate {
public:
    // strategy = how we find a spot (default: first available of the right type).
    explicit EntranceGate(const string& gateId,
                         shared_ptr<SpotFindingStrategy> strategy = make_shared<FirstAvailableStrategy>())
        : gateId_(gateId), ticketCount_(0), spotStrategy_(strategy) {}

    string getGateId() const { return gateId_; }

    // Issue a ticket only if the lot has space. Uses SpotFindingStrategy to pick a spot.
    shared_ptr<Ticket> issueTicket(ParkingLot& lot, shared_ptr<Vehicle> vehicle) {
        if (lot.isFull()) return nullptr;

        shared_ptr<ParkingSpot> spot = spotStrategy_->findSpot(lot, vehicle->getType());
        if (!spot) return nullptr;

        spot->assignVehicle(vehicle);
        lot.notifySpotChanged();

        ticketCount_++;
        string ticketId = gateId_ + "-T" + to_string(ticketCount_);
        long entryTime = time(0);
        // time(0) = current time as seconds since Jan 1 1970 (from <ctime>).
        shared_ptr<Ticket> ticket = make_shared<Ticket>(ticketId, entryTime, spot, vehicle);
        return ticket;
    }

private:
    string gateId_;
    int ticketCount_;
    shared_ptr<SpotFindingStrategy> spotStrategy_;
};

}  // namespace parking
