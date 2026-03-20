#pragma once

namespace parking {

enum class SpotType {
    MOTORCYCLE,
    COMPACT,
    LARGE,
    HANDICAPPED
};

enum class VehicleType {
    MOTORCYCLE,
    CAR,
    TRUCK,
    VAN
};

enum class TicketStatus {
    ACTIVE,
    PAID,
    LOST,
    EXPIRED
};

enum class PaymentType {
    CASH,
    CREDIT_CARD,
    UPI
};

enum class PaymentStatus {
    PENDING,
    COMPLETED,
    FAILED,
    REFUNDED
};

}  // namespace parking
