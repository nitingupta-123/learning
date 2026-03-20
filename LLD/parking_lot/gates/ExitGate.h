#pragma once

#include <ctime>
#include <memory>
#include <string>
#include "../core/ParkingLot.h"
#include "../core/Ticket.h"
#include "../enums/enums.h"

namespace parking {

using namespace std;

class ExitGate {
public:
    explicit ExitGate(const string& gateId) : gateId_(gateId) {}

    string getGateId() const { return gateId_; }

    // Scan ticket at exit: remove vehicle from spot, set exit time and status.
    // Pass lot so we can notify display observers (optional; use nullptr if no display update needed).
    void scanTicket(shared_ptr<Ticket> ticket, ParkingLot* lot = nullptr) {
        if (!ticket) return;
        shared_ptr<ParkingSpot> spot = ticket->getSpot();
        if (spot) {
            spot->removeVehicle();
            if (lot) lot->returnSpotToPool(spot);
        }
        if (lot) lot->notifySpotChanged();
        ticket->setExitTime(time(0));
        ticket->setStatus(TicketStatus::PAID);
    }

private:
    string gateId_;
};

}  // namespace parking
