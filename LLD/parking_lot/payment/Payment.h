#pragma once

#include <ctime>
#include <memory>
#include <string>
#include "../core/Ticket.h"
#include "../enums/enums.h"
#include "PricingStrategy.h"

namespace parking {

using namespace std;

// Payment is created when we want to pay for a ticket. It uses a PricingStrategy to compute the amount.
class Payment {
public:
    Payment(const string& paymentId, shared_ptr<PricingStrategy> strategy)
        : paymentId_(paymentId), amount_(0), paymentTime_(0), status_(PaymentStatus::PENDING),
          type_(PaymentType::CASH), pricingStrategy_(strategy) {}

    string getPaymentId() const { return paymentId_; }
    double getAmount() const { return amount_; }
    long getPaymentTime() const { return paymentTime_; }
    PaymentStatus getStatus() const { return status_; }
    PaymentType getType() const { return type_; }

    // Use the strategy to compute fee from the ticket (entry/exit time, spot type).
    double calculateAmount(shared_ptr<Ticket> ticket) {
        if (!ticket || !pricingStrategy_) return 0;
        shared_ptr<ParkingSpot> spot = ticket->getSpot();
        if (!spot) return 0;
        amount_ = pricingStrategy_->calculateFee(
            ticket->getEntryTime(), ticket->getExitTime(), spot->getType());
        return amount_;
    }

    // Mark payment as done with the given PaymentType. Returns true if we were PENDING.
    bool completePayment(shared_ptr<Ticket> ticket, PaymentType paymentType) {
        if (status_ != PaymentStatus::PENDING) return false;
        type_ = paymentType;
        paymentTime_ = time(0);
        status_ = PaymentStatus::COMPLETED;
        if (ticket) ticket->setStatus(TicketStatus::PAID);
        return true;
    }

private:
    string paymentId_;
    double amount_;
    long paymentTime_;
    PaymentStatus status_;
    PaymentType type_;
    shared_ptr<PricingStrategy> pricingStrategy_;
};

}  // namespace parking
