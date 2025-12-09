#pragma once

#include "matching_engine.h"
#include "event_sourcing.h"
#include "deterministic_calculator.h"
#include <memory>

namespace perpetual {

// Matching Engine with Event Sourcing and Deterministic Calculation
// This version emits all events to EventStore and uses deterministic calculations
class MatchingEngineEventSourcing : public MatchingEngine {
public:
    MatchingEngineEventSourcing(InstrumentID instrument_id, 
                                EventStore* event_store = nullptr);
    ~MatchingEngineEventSourcing();
    
    // Initialize with event store
    bool initialize(const std::string& event_store_dir);
    
    // Process order with event sourcing
    std::vector<Trade> process_order_es(Order* order);
    
    // Cancel order with event sourcing
    bool cancel_order_es(OrderID order_id, UserID user_id);
    
    // Replay events to rebuild state
    bool replay_events(SequenceID from = 0, SequenceID to = UINT64_MAX);
    
    // Get event store
    EventStore* get_event_store() const { return event_store_.get(); }
    
    // Get event publisher
    EventPublisher* get_event_publisher() const { return event_publisher_.get(); }
    
    // Use deterministic timestamp instead of system time
    void set_deterministic_mode(bool enabled) { deterministic_mode_ = enabled; }
    bool deterministic_mode() const { return deterministic_mode_; }
    
private:
    // Override match_order to use deterministic calculations
    std::vector<Trade> match_order_deterministic(Order* order);
    
    // Use deterministic calculator for price matching
    bool can_match_deterministic(const Order* taker, const Order* maker) const;
    
    // Use deterministic calculator for match price
    Price get_match_price_deterministic(const Order* taker, const Order* maker) const;
    
    // Use deterministic calculator for trade quantity
    Quantity get_trade_quantity_deterministic(const Order* taker, const Order* maker) const;
    
    // Emit events for order lifecycle
    void emit_order_placed_event(const Order& order);
    void emit_order_matched_event(OrderID taker_id, OrderID maker_id,
                                  Price price, Quantity quantity);
    void emit_order_cancelled_event(OrderID order_id, UserID user_id,
                                    OrderStatus old_status, OrderStatus new_status);
    void emit_trade_executed_event(const Trade& trade);
    void emit_order_rejected_event(OrderID order_id, UserID user_id,
                                   const std::string& reason);
    
    std::unique_ptr<EventStore> event_store_;
    std::unique_ptr<EventPublisher> event_publisher_;
    bool deterministic_mode_ = true;
    bool owns_event_store_ = false;
};

} // namespace perpetual

