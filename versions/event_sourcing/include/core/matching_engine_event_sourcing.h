#pragma once

#include "core/matching_engine.h"
#include "core/event_sourcing.h"
#include <memory>
#include <vector>

namespace perpetual {

// Forward declarations
class EventStore;
class EventPublisher;

// Matching Engine with Event Sourcing support
class MatchingEngineEventSourcing : public MatchingEngine {
public:
    MatchingEngineEventSourcing(InstrumentID instrument_id, 
                               EventStore* event_store = nullptr);
    ~MatchingEngineEventSourcing();
    
    // Initialize event store
    bool initialize(const std::string& event_store_dir);
    
    // Process order with event sourcing
    std::vector<Trade> process_order_es(Order* order);
    
    // Cancel order with event sourcing
    bool cancel_order_es(OrderID order_id, UserID user_id = 0);
    
    // Get event store
    EventStore* getEventStore() const { return event_store_.get(); }
    
    // Get event publisher
    EventPublisher* getEventPublisher() const { return event_publisher_.get(); }
    
protected:
    // Event store
    std::unique_ptr<EventStore> event_store_;
    bool owns_event_store_;
    
    // Event publisher
    std::unique_ptr<EventPublisher> event_publisher_;
    
    // Emit events
    void emit_order_placed_event(const Order& order);
    void emit_order_filled_event(const Trade& trade);
    void emit_order_partially_filled_event(const Order& order, const Trade& trade);
    void emit_order_cancelled_event(OrderID order_id, UserID user_id);
    void emit_order_cancelled_event(OrderID order_id, UserID user_id, OrderStatus old_status, OrderStatus new_status);
    void emit_order_matched_event(OrderID taker_id, OrderID maker_id, Price price, Quantity quantity);
    void emit_order_rejected_event(OrderID order_id, UserID user_id, const std::string& reason);
    void emit_trade_executed_event(const Trade& trade);
    
    // Replay events
    bool replay_events(SequenceID from, SequenceID to);
    
    // Deterministic matching helpers
    bool can_match_deterministic(const Order* taker, const Order* maker) const;
    Price get_match_price_deterministic(const Order* taker, const Order* maker) const;
    Quantity get_trade_quantity_deterministic(const Order* taker, const Order* maker) const;
    
    // Deterministic mode flag
    bool deterministic_mode_;
    
    // Deterministic matching
    std::vector<Trade> match_order_deterministic(Order* order);
};

} // namespace perpetual
