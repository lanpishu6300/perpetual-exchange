#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace perpetual {

// Basic types for high-performance trading
using OrderID = uint64_t;
using UserID = uint64_t;
using InstrumentID = uint32_t;
using Price = int64_t;  // Price in ticks (scaled integer for precision)
using Quantity = int64_t;  // Quantity in base units (scaled integer)
using Timestamp = int64_t;  // Nanoseconds since epoch
using SequenceID = uint64_t;

// Trading constants
constexpr Price PRICE_SCALE = 1000000000LL;  // 10^9, allowing 9 decimal places
constexpr Quantity QTY_SCALE = 1000000LL;    // 10^6, allowing 6 decimal places

// Order types
enum class OrderType : uint8_t {
    LIMIT = 0,
    MARKET = 1,
    IOC = 2,    // Immediate or Cancel
    FOK = 3     // Fill or Kill
};

// Order side
enum class OrderSide : uint8_t {
    BUY = 0,
    SELL = 1
};

// Order status
enum class OrderStatus : uint8_t {
    PENDING = 0,
    PARTIAL_FILLED = 1,
    FILLED = 2,
    CANCELLED = 3,
    REJECTED = 4
};

// Position direction
enum class PositionSide : uint8_t {
    LONG = 0,
    SHORT = 1,
    NET = 2
};

// Offset flag for futures/perpetual contracts
enum class OffsetFlag : uint8_t {
    OPEN = 0,       // Open position
    CLOSE = 1,      // Close position
    CLOSE_TODAY = 2, // Close today's position
    CLOSE_YESTERDAY = 3 // Close yesterday's position
};

// Helper functions for price/quantity conversion
inline double price_to_double(Price p) {
    return static_cast<double>(p) / PRICE_SCALE;
}

inline Price double_to_price(double p) {
    return static_cast<Price>(p * PRICE_SCALE);
}

inline double quantity_to_double(Quantity q) {
    return static_cast<double>(q) / QTY_SCALE;
}

inline Quantity double_to_quantity(double q) {
    return static_cast<Quantity>(q * QTY_SCALE);
}

// Get current timestamp in nanoseconds
inline Timestamp get_current_timestamp() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

} // namespace perpetual
