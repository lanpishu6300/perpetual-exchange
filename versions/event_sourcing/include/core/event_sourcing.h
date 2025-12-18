#pragma once

#include "core/types.h"
#include "core/order.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>
#include <atomic>
#include <shared_mutex>

namespace perpetual {

// Forward declaration
struct Trade;

// Event types for Event Sourcing
enum class EventType : uint8_t {
    ORDER_PLACED = 0,
    ORDER_MATCHED = 1,
    ORDER_CANCELLED = 2,
    ORDER_REJECTED = 3,
    TRADE_EXECUTED = 4,
    ORDER_PARTIALLY_FILLED = 5,
    ORDER_FULLY_FILLED = 6
};

// Base event structure for Event Sourcing
// All events are immutable and deterministic
struct alignas(64) Event {
    EventType type;
    SequenceID sequence_id;  // Deterministic sequence number
    Timestamp event_timestamp;  // Deterministic timestamp (based on sequence)
    InstrumentID instrument_id;
    
    // Event payload (union for type-specific data)
    union {
        struct {
            OrderID order_id;
            UserID user_id;
            OrderSide side;
            OrderType order_type;
            Price price;
            Quantity quantity;
        } order_placed;
        
        struct {
            OrderID taker_order_id;
            OrderID maker_order_id;
            Price match_price;
            Quantity match_quantity;
        } order_matched;
        
        struct {
            OrderID order_id;
            UserID user_id;
            OrderStatus old_status;
            OrderStatus new_status;
        } order_cancelled;
        
        struct {
            OrderID order_id;
            UserID user_id;
            char reason[256];  // Fixed-size string to avoid union issues
        } order_rejected;
        
        struct {
            OrderID buy_order_id;
            OrderID sell_order_id;
            UserID buy_user_id;
            UserID sell_user_id;
            InstrumentID instrument_id;
            Price price;
            Quantity quantity;
            Timestamp timestamp;
            SequenceID sequence_id;
            bool is_taker_buy;
        } trade_executed;
    } data;
    
    // Serialization for persistence
    std::string serialize() const;
    static Event deserialize(const std::string& data);
    
    // Deterministic hash for verification
    uint64_t hash() const;
    
    // Default constructor (required for containers)
    Event() : type(EventType::ORDER_PLACED), sequence_id(0), event_timestamp(0), instrument_id(0) {
        // Initialize union with first member
        data.order_placed = {};
    }
    
    // Copy constructor
    Event(const Event& other) 
        : type(other.type), sequence_id(other.sequence_id), 
          event_timestamp(other.event_timestamp), instrument_id(other.instrument_id) {
        data = other.data;
    }
    
    // Move constructor
    Event(Event&& other) noexcept
        : type(other.type), sequence_id(other.sequence_id),
          event_timestamp(other.event_timestamp), instrument_id(other.instrument_id) {
        data = other.data;
    }
    
    // Assignment operators
    Event& operator=(const Event& other) {
        if (this != &other) {
            type = other.type;
            sequence_id = other.sequence_id;
            event_timestamp = other.event_timestamp;
            instrument_id = other.instrument_id;
            data = other.data;
        }
        return *this;
    }
    
    // Destructor (trivial - no cleanup needed with fixed-size arrays)
    ~Event() = default;
};

// Event Store for Event Sourcing
// Stores all events in append-only log
class EventStore {
public:
    EventStore();
    ~EventStore();
    
    // Initialize event store
    bool initialize(const std::string& data_dir);
    
    // Append event (immutable, append-only)
    bool append_event(const Event& event);
    
    // Get events by sequence range
    std::vector<Event> get_events(SequenceID from, SequenceID to) const;
    
    // Get all events for an order
    std::vector<Event> get_order_events(OrderID order_id) const;
    
    // Get all events for an instrument
    std::vector<Event> get_instrument_events(InstrumentID instrument_id, 
                                             SequenceID from = 0, 
                                             SequenceID to = UINT64_MAX) const;
    
    // Get latest sequence number
    SequenceID get_latest_sequence() const { return latest_sequence_; }
    
    // Replay events (for recovery and testing)
    // Returns true if replay was successful
    bool replay_events(SequenceID from, SequenceID to,
                      std::function<bool(const Event&)> handler) const;
    
    // Create snapshot at a specific sequence
    bool create_snapshot(SequenceID sequence, const std::string& snapshot_path);
    
    // Load snapshot
    bool load_snapshot(const std::string& snapshot_path, SequenceID& sequence);
    
    // Flush pending writes
    void flush();
    
    // Get event count
    size_t event_count() const { return event_count_; }
    
private:
    void write_event_to_log(const Event& event);
    Event read_event_from_log(std::ifstream& file) const;
    
    std::string data_dir_;
    std::string event_log_path_;
    std::unique_ptr<std::ofstream> event_log_;
    std::mutex event_log_mutex_;
    
    std::atomic<SequenceID> latest_sequence_{0};
    std::atomic<size_t> event_count_{0};
    
    // Indexes for fast lookup
    std::unordered_map<OrderID, std::vector<SequenceID>> order_index_;
    std::unordered_map<InstrumentID, std::vector<SequenceID>> instrument_index_;
    mutable std::shared_mutex index_mutex_;
    
    bool initialized_ = false;
};

// Event Publisher for emitting events from matching engine
class EventPublisher {
public:
    EventPublisher(EventStore* event_store);
    
    // Publish events (non-blocking, async)
    void publish_order_placed(const Order& order);
    void publish_order_matched(OrderID taker_id, OrderID maker_id, 
                               Price price, Quantity quantity);
    void publish_order_cancelled(OrderID order_id, UserID user_id,
                                 OrderStatus old_status, OrderStatus new_status);
    void publish_order_rejected(OrderID order_id, UserID user_id, 
                                const std::string& reason);
    void publish_trade_executed(const Trade& trade);
    void publish_order_status_changed(OrderID order_id, OrderStatus old_status, 
                                      OrderStatus new_status);
    
    // Flush pending events
    void flush();
    
private:
    EventStore* event_store_;
    
    // Helper to create event
    Event create_event(EventType type, SequenceID sequence, InstrumentID instrument_id);
};
} // namespace perpetual
