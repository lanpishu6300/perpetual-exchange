#include "core/event_sourcing.h"
#include "core/deterministic_calculator.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <shared_mutex>
#include <mutex>
#include <unordered_map>
#include <cstring>
#include <stdexcept>

namespace perpetual {

namespace {
// Safe string to number conversion helpers
unsigned long safe_stoul(const std::string& str) {
    try {
        return std::stoul(str);
    } catch (...) {
        return 0;
    }
}

unsigned long long safe_stoull(const std::string& str) {
    try {
        return std::stoull(str);
    } catch (...) {
        return 0;
    }
}

long long safe_stoll(const std::string& str) {
    try {
        return std::stoll(str);
    } catch (...) {
        return 0;
    }
}
} // anonymous namespace

// Event serialization
std::string Event::serialize() const {
    std::ostringstream oss;
    oss << static_cast<uint8_t>(type) << ","
        << sequence_id << ","
        << event_timestamp << ","
        << instrument_id << ",";
    
    switch (type) {
        case EventType::ORDER_PLACED:
            oss << data.order_placed.order_id << ","
                << data.order_placed.user_id << ","
                << static_cast<uint8_t>(data.order_placed.side) << ","
                << static_cast<uint8_t>(data.order_placed.order_type) << ","
                << data.order_placed.price << ","
                << data.order_placed.quantity;
            break;
        case EventType::ORDER_MATCHED:
            oss << data.order_matched.taker_order_id << ","
                << data.order_matched.maker_order_id << ","
                << data.order_matched.match_price << ","
                << data.order_matched.match_quantity;
            break;
        case EventType::ORDER_CANCELLED:
            oss << data.order_cancelled.order_id << ","
                << data.order_cancelled.user_id << ","
                << static_cast<uint8_t>(data.order_cancelled.old_status) << ","
                << static_cast<uint8_t>(data.order_cancelled.new_status);
            break;
        case EventType::ORDER_REJECTED:
            oss << data.order_rejected.order_id << ","
                << data.order_rejected.user_id << ","
                << std::string(data.order_rejected.reason);
            break;
        case EventType::TRADE_EXECUTED:
            oss << data.trade_executed.buy_order_id << ","
                << data.trade_executed.sell_order_id << ","
                << data.trade_executed.buy_user_id << ","
                << data.trade_executed.sell_user_id << ","
                << data.trade_executed.instrument_id << ","
                << data.trade_executed.price << ","
                << data.trade_executed.quantity << ","
                << data.trade_executed.timestamp << ","
                << data.trade_executed.sequence_id << ","
                << (data.trade_executed.is_taker_buy ? 1 : 0);
            break;
        default:
            break;
    }
    
    return oss.str();
}

// Event deserialization
Event Event::deserialize(const std::string& data_str) {
    Event event;
    
    // Check if data is empty
    if (data_str.empty()) {
        return event;  // Return default event
    }
    
    std::istringstream iss(data_str);
    std::string token;
    
    // Parse type
    if (!std::getline(iss, token, ',')) {
        return event;
    }
    event.type = static_cast<EventType>(safe_stoul(token));
    
    // Parse sequence_id
    if (!std::getline(iss, token, ',')) {
        return event;
    }
    event.sequence_id = safe_stoull(token);
    
    // Parse timestamp
    if (!std::getline(iss, token, ',')) {
        return event;
    }
    event.event_timestamp = safe_stoll(token);
    
    // Parse instrument_id
    if (!std::getline(iss, token, ',')) {
        return event;
    }
    event.instrument_id = safe_stoul(token);
    
    // Parse type-specific data
    switch (event.type) {
        case EventType::ORDER_PLACED:
            if (!std::getline(iss, token, ',')) break;
            event.data.order_placed.order_id = safe_stoull(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.order_placed.user_id = safe_stoull(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.order_placed.side = static_cast<OrderSide>(safe_stoul(token));
            if (!std::getline(iss, token, ',')) break;
            event.data.order_placed.order_type = static_cast<OrderType>(safe_stoul(token));
            if (!std::getline(iss, token, ',')) break;
            event.data.order_placed.price = safe_stoll(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.order_placed.quantity = safe_stoll(token);
            break;
        case EventType::ORDER_MATCHED:
            if (!std::getline(iss, token, ',')) break;
            event.data.order_matched.taker_order_id = safe_stoull(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.order_matched.maker_order_id = safe_stoull(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.order_matched.match_price = safe_stoll(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.order_matched.match_quantity = safe_stoll(token);
            break;
        case EventType::ORDER_CANCELLED:
            if (!std::getline(iss, token, ',')) break;
            event.data.order_cancelled.order_id = safe_stoull(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.order_cancelled.user_id = safe_stoull(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.order_cancelled.old_status = static_cast<OrderStatus>(safe_stoul(token));
            if (!std::getline(iss, token, ',')) break;
            event.data.order_cancelled.new_status = static_cast<OrderStatus>(safe_stoul(token));
            break;
        case EventType::ORDER_REJECTED:
            if (!std::getline(iss, token, ',')) break;
            event.data.order_rejected.order_id = safe_stoull(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.order_rejected.user_id = safe_stoull(token);
            std::getline(iss, token);
            strncpy(event.data.order_rejected.reason, token.c_str(), sizeof(event.data.order_rejected.reason) - 1);
            event.data.order_rejected.reason[sizeof(event.data.order_rejected.reason) - 1] = '\0';
            break;
        case EventType::TRADE_EXECUTED:
            if (!std::getline(iss, token, ',')) break;
            event.data.trade_executed.buy_order_id = safe_stoull(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.trade_executed.sell_order_id = safe_stoull(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.trade_executed.buy_user_id = safe_stoull(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.trade_executed.sell_user_id = safe_stoull(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.trade_executed.instrument_id = safe_stoul(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.trade_executed.price = safe_stoll(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.trade_executed.quantity = safe_stoll(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.trade_executed.timestamp = safe_stoll(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.trade_executed.sequence_id = safe_stoull(token);
            if (!std::getline(iss, token, ',')) break;
            event.data.trade_executed.is_taker_buy = (safe_stoul(token) == 1);
            break;
        default:
            break;
    }
    
    return event;
}

// Deterministic hash
uint64_t Event::hash() const {
    uint64_t h = static_cast<uint64_t>(type);
    h = h * 31 + sequence_id;
    h = h * 31 + event_timestamp;
    h = h * 31 + instrument_id;
    
    switch (type) {
        case EventType::ORDER_PLACED:
            h = h * 31 + data.order_placed.order_id;
            h = h * 31 + data.order_placed.user_id;
            break;
        case EventType::ORDER_MATCHED:
            h = h * 31 + data.order_matched.taker_order_id;
            h = h * 31 + data.order_matched.maker_order_id;
            break;
        case EventType::TRADE_EXECUTED:
            h = h * 31 + data.trade_executed.buy_order_id;
            h = h * 31 + data.trade_executed.sell_order_id;
            break;
        default:
            break;
    }
    
    return h;
}

// EventStore implementation
EventStore::EventStore() {
}

EventStore::~EventStore() {
    flush();
    if (event_log_) {
        event_log_->close();
    }
}

bool EventStore::initialize(const std::string& data_dir) {
    data_dir_ = data_dir;
    event_log_path_ = data_dir + "/events.log";
    
    // Open event log in append mode
    event_log_ = std::make_unique<std::ofstream>(event_log_path_, 
                                                 std::ios::app | std::ios::binary);
    if (!event_log_->is_open()) {
        return false;
    }
    
    // Read existing events to rebuild indexes and get latest sequence
    std::ifstream read_file(event_log_path_, std::ios::binary);
    if (read_file.is_open()) {
        SequenceID max_seq = 0;
        size_t count = 0;
        while (read_file.peek() != EOF) {
            Event event = read_event_from_log(read_file);
            if (event.sequence_id > max_seq) {
                max_seq = event.sequence_id;
            }
            
            // Rebuild indexes
            if (event.type == EventType::ORDER_PLACED || 
                event.type == EventType::ORDER_MATCHED ||
                event.type == EventType::ORDER_CANCELLED) {
                OrderID order_id = (event.type == EventType::ORDER_PLACED) ?
                    event.data.order_placed.order_id :
                    (event.type == EventType::ORDER_MATCHED) ?
                    event.data.order_matched.taker_order_id :
                    event.data.order_cancelled.order_id;
                order_index_[order_id].push_back(event.sequence_id);
            }
            instrument_index_[event.instrument_id].push_back(event.sequence_id);
            count++;
        }
        latest_sequence_ = max_seq;
        event_count_ = count;
    }
    
    initialized_ = true;
    return true;
}

bool EventStore::append_event(const Event& event) {
    if (!initialized_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(event_log_mutex_);
    
    // Assign sequence if not set
    Event event_copy = event;
    if (event_copy.sequence_id == 0) {
        event_copy.sequence_id = ++latest_sequence_;
    } else if (event_copy.sequence_id > latest_sequence_) {
        latest_sequence_ = event_copy.sequence_id;
    }
    
    // Use deterministic timestamp from sequence
    if (event_copy.event_timestamp == 0) {
        event_copy.event_timestamp = DeterministicCalculator::sequence_to_timestamp(
            event_copy.sequence_id);
    }
    
    write_event_to_log(event_copy);
    
    // Update indexes
    {
        std::unique_lock<std::shared_mutex> index_lock(index_mutex_);
        if (event_copy.type == EventType::ORDER_PLACED || 
            event_copy.type == EventType::ORDER_MATCHED ||
            event_copy.type == EventType::ORDER_CANCELLED) {
            OrderID order_id = (event_copy.type == EventType::ORDER_PLACED) ?
                event_copy.data.order_placed.order_id :
                (event_copy.type == EventType::ORDER_MATCHED) ?
                event_copy.data.order_matched.taker_order_id :
                event_copy.data.order_cancelled.order_id;
            order_index_[order_id].push_back(event_copy.sequence_id);
        }
        instrument_index_[event_copy.instrument_id].push_back(event_copy.sequence_id);
    }
    
    event_count_++;
    return true;
}

std::vector<Event> EventStore::get_events(SequenceID from, SequenceID to) const {
    std::vector<Event> events;
    
    std::ifstream file(event_log_path_, std::ios::binary);
    if (!file.is_open()) {
        return events;
    }
    
    while (file.peek() != EOF) {
        Event event = read_event_from_log(file);
        if (event.sequence_id >= from && event.sequence_id <= to) {
            events.push_back(event);
        }
        if (event.sequence_id > to) {
            break;
        }
    }
    
    return events;
}

std::vector<Event> EventStore::get_order_events(OrderID order_id) const {
    std::vector<Event> events;
    
    std::shared_lock<std::shared_mutex> lock(index_mutex_);
    auto it = order_index_.find(order_id);
    if (it == order_index_.end()) {
        return events;
    }
    
    std::ifstream file(event_log_path_, std::ios::binary);
    if (!file.is_open()) {
        return events;
    }
    
    // Read all events and filter by order_id
    while (file.peek() != EOF) {
        Event event = read_event_from_log(file);
        if (event.sequence_id == 0) continue;
        
        OrderID event_order_id = 0;
        if (event.type == EventType::ORDER_PLACED) {
            event_order_id = event.data.order_placed.order_id;
        } else if (event.type == EventType::ORDER_MATCHED) {
            if (event.data.order_matched.taker_order_id == order_id ||
                event.data.order_matched.maker_order_id == order_id) {
                events.push_back(event);
            }
            continue;
        } else if (event.type == EventType::ORDER_CANCELLED) {
            event_order_id = event.data.order_cancelled.order_id;
        }
        
        if (event_order_id == order_id) {
            events.push_back(event);
        }
    }
    
    return events;
}

std::vector<Event> EventStore::get_instrument_events(InstrumentID instrument_id, 
                                                     SequenceID from, 
                                                     SequenceID to) const {
    std::vector<Event> events;
    
    std::ifstream file(event_log_path_, std::ios::binary);
    if (!file.is_open()) {
        return events;
    }
    
    while (file.peek() != EOF) {
        Event event = read_event_from_log(file);
        if (event.instrument_id == instrument_id &&
            event.sequence_id >= from && event.sequence_id <= to) {
            events.push_back(event);
        }
        if (event.sequence_id > to) {
            break;
        }
    }
    
    return events;
}

bool EventStore::replay_events(SequenceID from, SequenceID to,
                              std::function<bool(const Event&)> handler) const {
    std::ifstream file(event_log_path_, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    while (file.peek() != EOF) {
        Event event = read_event_from_log(file);
        if (event.sequence_id >= from && event.sequence_id <= to) {
            if (!handler(event)) {
                return false;  // Handler requested stop
            }
        }
        if (event.sequence_id > to) {
            break;
        }
    }
    
    return true;
}

bool EventStore::create_snapshot(SequenceID sequence, const std::string& snapshot_path) {
    // Snapshot creation would serialize current state
    // For now, just mark the sequence
    std::ofstream snapshot(snapshot_path);
    if (!snapshot.is_open()) {
        return false;
    }
    snapshot << sequence << std::endl;
    return true;
}

bool EventStore::load_snapshot(const std::string& snapshot_path, SequenceID& sequence) {
    std::ifstream snapshot(snapshot_path);
    if (!snapshot.is_open()) {
        return false;
    }
    snapshot >> sequence;
    return true;
}

void EventStore::flush() {
    std::lock_guard<std::mutex> lock(event_log_mutex_);
    if (event_log_ && event_log_->is_open()) {
        event_log_->flush();
    }
}

void EventStore::write_event_to_log(const Event& event) {
    if (event_log_ && event_log_->is_open()) {
        std::string serialized = event.serialize();
        size_t len = serialized.length();
        event_log_->write(reinterpret_cast<const char*>(&len), sizeof(len));
        event_log_->write(serialized.c_str(), len);
        event_log_->flush();
    }
}

Event EventStore::read_event_from_log(std::ifstream& file) const {
    Event event;
    size_t len = 0;
    
    if (!file.read(reinterpret_cast<char*>(&len), sizeof(len))) {
        return event;  // EOF or error
    }
    
    std::string data(len, '\0');
    if (!file.read(&data[0], len)) {
        return event;  // Error
    }
    
    event = Event::deserialize(data);
    return event;
}

// EventPublisher implementation
EventPublisher::EventPublisher(EventStore* event_store) 
    : event_store_(event_store) {
}

void EventPublisher::publish_order_placed(const Order& order) {
    Event event;
    event.type = EventType::ORDER_PLACED;
    event.instrument_id = order.instrument_id;
    event.data.order_placed.order_id = order.order_id;
    event.data.order_placed.user_id = order.user_id;
    event.data.order_placed.side = order.side;
    event.data.order_placed.order_type = order.order_type;
    event.data.order_placed.price = order.price;
    event.data.order_placed.quantity = order.quantity;
    
    event_store_->append_event(event);
}

void EventPublisher::publish_order_matched(OrderID taker_id, OrderID maker_id,
                                          Price price, Quantity quantity) {
    Event event;
    event.type = EventType::ORDER_MATCHED;
    event.data.order_matched.taker_order_id = taker_id;
    event.data.order_matched.maker_order_id = maker_id;
    event.data.order_matched.match_price = price;
    event.data.order_matched.match_quantity = quantity;
    
    event_store_->append_event(event);
}

void EventPublisher::publish_order_cancelled(OrderID order_id, UserID user_id,
                                             OrderStatus old_status, OrderStatus new_status) {
    Event event;
    event.type = EventType::ORDER_CANCELLED;
    event.data.order_cancelled.order_id = order_id;
    event.data.order_cancelled.user_id = user_id;
    event.data.order_cancelled.old_status = old_status;
    event.data.order_cancelled.new_status = new_status;
    
    event_store_->append_event(event);
}

void EventPublisher::publish_order_rejected(OrderID order_id, UserID user_id,
                                           const std::string& reason) {
    Event event;
    event.type = EventType::ORDER_REJECTED;
    event.data.order_rejected.order_id = order_id;
    event.data.order_rejected.user_id = user_id;
    strncpy(event.data.order_rejected.reason, reason.c_str(), sizeof(event.data.order_rejected.reason) - 1);
    event.data.order_rejected.reason[sizeof(event.data.order_rejected.reason) - 1] = '\0';
    
    event_store_->append_event(event);
}

void EventPublisher::publish_trade_executed(const Trade& trade) {
    Event event;
    event.type = EventType::TRADE_EXECUTED;
    event.instrument_id = trade.instrument_id;
    event.data.trade_executed.buy_order_id = trade.buy_order_id;
    event.data.trade_executed.sell_order_id = trade.sell_order_id;
    event.data.trade_executed.buy_user_id = trade.buy_user_id;
    event.data.trade_executed.sell_user_id = trade.sell_user_id;
    event.data.trade_executed.instrument_id = trade.instrument_id;
    event.data.trade_executed.price = trade.price;
    event.data.trade_executed.quantity = trade.quantity;
    event.data.trade_executed.timestamp = trade.timestamp;
    event.data.trade_executed.sequence_id = trade.sequence_id;
    event.data.trade_executed.is_taker_buy = trade.is_taker_buy;
    
    event_store_->append_event(event);
}

void EventPublisher::publish_order_status_changed(OrderID order_id, OrderStatus old_status,
                                                  OrderStatus new_status) {
    if (new_status == OrderStatus::FILLED) {
        // Already handled by trade_executed
        return;
    }
    
    Event event;
    if (new_status == OrderStatus::CANCELLED) {
        event.type = EventType::ORDER_CANCELLED;
        event.data.order_cancelled.order_id = order_id;
        event.data.order_cancelled.old_status = old_status;
        event.data.order_cancelled.new_status = new_status;
    } else if (new_status == OrderStatus::REJECTED) {
        event.type = EventType::ORDER_REJECTED;
        event.data.order_rejected.order_id = order_id;
        strncpy(event.data.order_rejected.reason, "Status changed", sizeof(event.data.order_rejected.reason) - 1);
        event.data.order_rejected.reason[sizeof(event.data.order_rejected.reason) - 1] = '\0';
    }
    
    if (event.type != EventType::ORDER_PLACED) {  // Valid event type
        event_store_->append_event(event);
    }
}

void EventPublisher::flush() {
    if (event_store_) {
        event_store_->flush();
    }
}

Event EventPublisher::create_event(EventType type, SequenceID sequence, InstrumentID instrument_id) {
    Event event;
    event.type = type;
    event.sequence_id = sequence;
    event.instrument_id = instrument_id;
    event.event_timestamp = DeterministicCalculator::sequence_to_timestamp(sequence);
    return event;
}

} // namespace perpetual
