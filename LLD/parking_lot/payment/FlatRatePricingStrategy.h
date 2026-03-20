#pragma once

#include "PricingStrategy.h"

namespace parking {

using namespace std;

// One fixed fee regardless of how long the vehicle stayed.
class FlatRatePricingStrategy : public PricingStrategy {
public:
    FlatRatePricingStrategy(double flatFee) : flatFee_(flatFee) {}

    double calculateFee(long entryTime, long exitTime, SpotType spotType) override {
        (void)entryTime;
        (void)exitTime;
        (void)spotType;
        return flatFee_;
    }

private:
    double flatFee_;
};

}  // namespace parking
