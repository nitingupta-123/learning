// Demo: create lot -> add floors and spots -> park a car -> issue ticket -> process payment -> exit.

#include <iostream>
#include <memory>

#include "core/ParkingLot.h"
#include "core/ParkingFloor.h"
#include "core/Ticket.h"
#include "gates/EntranceGate.h"
#include "gates/ExitGate.h"
#include "payment/Payment.h"
#include "payment/HourlyPricingStrategy.h"
#include "display/GlobalDisplayBoard.h"
#include "spot/CompactSpot.h"
#include "vehicle/Car.h"

using namespace parking;
using namespace std;

int main() {
    // 1. Get the singleton lot and set name
    ParkingLot& lot = ParkingLot::getInstance();
    lot.setName("Main Parking Lot");

    // 2. Create a floor and add spots
    shared_ptr<ParkingFloor> floor = make_shared<ParkingFloor>(1);
    floor->addSpot(make_shared<CompactSpot>("C-101"));
    floor->addSpot(make_shared<CompactSpot>("C-102"));
    lot.addFloor(floor);

    // 3. Register display board (observer): it will be updated when a vehicle enters or exits.
    lot.addObserver(&GlobalDisplayBoard::getInstance());

    // 4. Add an entrance gate
    shared_ptr<EntranceGate> entranceGate = make_shared<EntranceGate>("E1");
    lot.addEntranceGate(entranceGate);

    // 5. Car arrives; issue ticket only if lot has space
    shared_ptr<Vehicle> car = make_shared<Car>("MH-12-AB-1234");
    shared_ptr<Ticket> ticket;

    if (!lot.isFull()) {
        ticket = entranceGate->issueTicket(lot, car);
        if (ticket) {
            cout << "Ticket issued: " << ticket->getTicketId()
                 << ", spot: " << ticket->getSpot()->getSpotId()
                 << ", entry: " << ticket->getEntryTime() << endl;
            cout << "Display updated: COMPACT free = " << GlobalDisplayBoard::getInstance().getCount(SpotType::COMPACT) << endl;
        }
    }

    if (!ticket) {
        cout << "No ticket (lot full or no spot)." << endl;
        return 0;
    }

    // 6. Simulate exit time (e.g. 2 hours later) and set it on ticket for fee calculation
    ticket->setExitTime(ticket->getEntryTime() + 7200);
    ticket->setStatus(TicketStatus::ACTIVE);

    // 7. Process payment
    shared_ptr<PricingStrategy> strategy = make_shared<HourlyPricingStrategy>(50.0);
    Payment payment("PAY-001", strategy);
    payment.calculateAmount(ticket);
    cout << "Amount to pay: " << payment.getAmount() << endl;

    if (payment.completePayment(ticket, PaymentType::UPI)) {
        cout << "Payment completed (UPI)." << endl;
    }

    // 8. Exit: scan ticket (pass &lot so display is updated when spot is freed)
    ExitGate exitGate("X1");
    exitGate.scanTicket(ticket, &lot);
    cout << "Vehicle exited. Ticket status: PAID." << endl;
    cout << "Display updated: COMPACT free = " << GlobalDisplayBoard::getInstance().getCount(SpotType::COMPACT) << endl;

    return 0;
}
