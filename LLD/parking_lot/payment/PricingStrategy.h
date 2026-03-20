#pragma once

#include "../enums/enums.h"

namespace parking {

using namespace std;

// Pure abstract class (interface): no implementation, only the method we must support.
// "= 0" at the end of a virtual function = pure virtual; we cannot create PricingStrategy objects,
// only objects of subclasses (HourlyPricingStrategy, FlatRatePricingStrategy) that implement calculateFee.
class PricingStrategy {
public:
    virtual ~PricingStrategy() {}

    // Compute fee from entry/exit time and spot type. Each strategy (hourly, flat) implements its own formula.
    virtual double calculateFee(long entryTime, long exitTime, SpotType spotType) = 0;
};

}  // namespace parking
