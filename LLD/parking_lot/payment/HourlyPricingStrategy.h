#pragma once

#include "PricingStrategy.h"

namespace parking {

using namespace std;

// Fee = (hours parked) * (rate per hour for this spot type). Simple formula.
class HourlyPricingStrategy : public PricingStrategy {
public:
    HourlyPricingStrategy(double ratePerHour) : ratePerHour_(ratePerHour) {}

    double calculateFee(long entryTime, long exitTime, SpotType spotType) override {
        // override = we are implementing the abstract method from PricingStrategy.
        if (exitTime <= entryTime) return 0;
        double hours = (exitTime - entryTime) / 3600.0;
        if (hours < 1.0) hours = 1.0;
        return hours * ratePerHour_;
    }

private:
    double ratePerHour_;
};

}  // namespace parking
