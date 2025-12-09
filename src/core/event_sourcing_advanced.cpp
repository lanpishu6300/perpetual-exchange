#include "core/event_sourcing_advanced.h"
#include "core/matching_engine_event_sourcing.h"
#include "core/orderbook.h"
#include "core/deterministic_calculator.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>

namespace perpetual {

// ============================================================================
// 1. Event Compression Implementation
// ============================================================================

EventCompressor::EventCompressor(EventStore* event_store)
    : event_store_(event_store) {
}

EventCompressor::~EventCompressor() {
    stop_background_compression();
}

size_t EventCompressor::compress_events(SequenceID up_to_sequence) {
    if (!event_store_) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(compression_mutex_);
    
    size_t compressed_count = 0;
    
    switch (strategy_) {
        case CompressionStrategy::SNAPSHOT_ONLY: {
            // Create snapshot but keep events
            std::string snapshot_path = "./snapshot_" + 
                                       std::to_string(up_to_sequence) + ".snap";
            create_snapshot_and_compress(up_to_sequence, snapshot_path);
            compressed_count = up_to_sequence;
            break;
        }
        case CompressionStrategy::SNAPSHOT_AND_DELETE: {
            // Create snapshot and mark events for deletion
            std::string snapshot_path = "./snapshot_" + 
                                       std::to_string(up_to_sequence) + ".snap";
            if (create_snapshot_and_compress(up_to_sequence, snapshot_path)) {
                // In production, would delete events from log file
                compressed_count = up_to_sequence;
            }
            break;
        }
        case CompressionStrategy::ARCHIVE: {
            // Archive events to compressed file
            std::string archive_path = "./archive_" + 
                                     std::to_string(up_to_sequence) + ".arc";
            if (archive_events(0, up_to_sequence, archive_path)) {
                compressed_count = up_to_sequence;
            }
            break;
        }
        default:
            break;
    }
    
    return compressed_count;
}

bool EventCompressor::create_snapshot_and_compress(SequenceID sequence, 
                                                   const std::string& snapshot_path) {
    if (!event_store_) {
        return false;
    }
    
    return event_store_->create_snapshot(sequence, snapshot_path);
}

bool EventCompressor::archive_events(SequenceID from, SequenceID to, 
                                    const std::string& archive_path) {
    if (!event_store_) {
        return false;
    }
    
    // Get events to archive
    auto events = event_store_->get_events(from, to);
    
    // Write to archive file (compressed)
    std::ofstream archive(archive_path, std::ios::binary);
    if (!archive.is_open()) {
        return false;
    }
    
    // Write header
    archive.write(reinterpret_cast<const char*>(&from), sizeof(from));
    archive.write(reinterpret_cast<const char*>(&to), sizeof(to));
    size_t count = events.size();
    archive.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    // Write events
    for (const auto& event : events) {
        std::string serialized = event.serialize();
        size_t len = serialized.length();
        archive.write(reinterpret_cast<const char*>(&len), sizeof(len));
        archive.write(serialized.c_str(), len);
    }
    
    return true;
}

bool EventCompressor::load_archived_events(const std::string& archive_path, 
                                          std::vector<Event>& events) {
    std::ifstream archive(archive_path, std::ios::binary);
    if (!archive.is_open()) {
        return false;
    }
    
    // Read header
    SequenceID from, to;
    size_t count;
    archive.read(reinterpret_cast<char*>(&from), sizeof(from));
    archive.read(reinterpret_cast<char*>(&to), sizeof(to));
    archive.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    // Read events
    events.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        size_t len = 0;
        if (!archive.read(reinterpret_cast<char*>(&len), sizeof(len))) {
            break;
        }
        
        std::string data(len, '\0');
        if (!archive.read(&data[0], len)) {
            break;
        }
        
        events.push_back(Event::deserialize(data));
    }
    
    return true;
}

void EventCompressor::start_background_compression() {
    if (running_) {
        return;
    }
    
    running_ = true;
    compression_thread_ = std::thread(&EventCompressor::compression_worker, this);
}

void EventCompressor::stop_background_compression() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    compression_cv_.notify_all();
    
    if (compression_thread_.joinable()) {
        compression_thread_.join();
    }
}

void EventCompressor::compression_worker() {
    SequenceID last_compressed = 0;
    
    while (running_) {
        std::unique_lock<std::mutex> lock(compression_mutex_);
        
        // Wait for compression interval or stop signal
        compression_cv_.wait_for(lock, std::chrono::minutes(10), [this] {
            return !running_;
        });
        
        if (!running_) {
            break;
        }
        
        if (event_store_) {
            SequenceID current_sequence = event_store_->get_latest_sequence();
            SequenceID compress_to = current_sequence - compression_interval_;
            
            if (compress_to > last_compressed && compress_to > retention_sequence_) {
                compress_events(compress_to);
                last_compressed = compress_to;
            }
        }
    }
}

// ============================================================================
// 2. Distributed Event Store Implementation
// ============================================================================

DistributedEventStore::DistributedEventStore(const DistributedEventStoreConfig& config)
    : config_(config) {
    // Initialize node availability
    for (NodeID node_id : config_.replica_nodes) {
        node_availability_[node_id] = true;  // Assume available initially
    }
}

DistributedEventStore::~DistributedEventStore() {
    if (replication_running_) {
        replication_running_ = false;
        if (replication_thread_.joinable()) {
            replication_thread_.join();
        }
    }
}

bool DistributedEventStore::initialize(const std::string& data_dir) {
    local_store_ = std::make_unique<EventStore>();
    if (!local_store_->initialize(data_dir)) {
        return false;
    }
    
    // Start replication thread
    replication_running_ = true;
    replication_thread_ = std::thread(&DistributedEventStore::replication_worker, this);
    
    return true;
}

bool DistributedEventStore::append_event(const Event& event) {
    // Append to local store
    if (!local_store_->append_event(event)) {
        return false;
    }
    
    // Queue for replication
    {
        std::lock_guard<std::mutex> lock(replication_mutex_);
        replication_queue_.push(event);
    }
    
    return true;
}

std::vector<Event> DistributedEventStore::get_events(SequenceID from, SequenceID to) const {
    // Try local store first
    auto events = local_store_->get_events(from, to);
    
    // If consensus enabled, verify with other nodes
    if (config_.enable_consensus && events.size() > 0) {
        // In production, would query multiple nodes and compare
        // For now, just return local events
    }
    
    return events;
}

std::vector<Event> DistributedEventStore::get_events_from_node(NodeID node_id, 
                                                              SequenceID from, 
                                                              SequenceID to) const {
    // In production, would make network call to node
    // For now, if it's local node, return local events
    if (node_id == config_.node_id) {
        return local_store_->get_events(from, to);
    }
    
    return {};
}

bool DistributedEventStore::replicate_event(const Event& event, 
                                            const std::vector<NodeID>& target_nodes) {
    // In production, would send event to target nodes via network
    // For now, just mark as replicated
    return true;
}

SequenceID DistributedEventStore::get_consensus_sequence() const {
    // In production, would query multiple nodes and get majority consensus
    // For now, return local sequence
    return local_store_->get_latest_sequence();
}

bool DistributedEventStore::is_node_available(NodeID node_id) const {
    std::shared_lock<std::shared_mutex> lock(node_mutex_);
    auto it = node_availability_.find(node_id);
    if (it != node_availability_.end()) {
        return it->second;
    }
    return false;
}

std::string DistributedEventStore::get_data_dir() const {
    // In production, would return actual data directory
    return "./";
}

void DistributedEventStore::replication_worker() {
    while (replication_running_) {
        std::vector<Event> events_to_replicate;
        
        // Batch events for replication
        {
            std::lock_guard<std::mutex> lock(replication_mutex_);
            while (!replication_queue_.empty() && events_to_replicate.size() < 100) {
                events_to_replicate.push_back(replication_queue_.front());
                replication_queue_.pop();
            }
        }
        
        // Replicate to other nodes
        if (!events_to_replicate.empty()) {
            for (const auto& event : events_to_replicate) {
                replicate_event(event, config_.replica_nodes);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ============================================================================
// 3. Event Stream Processing Implementation
// ============================================================================

EventStreamProcessor::EventStreamProcessor(EventStore* event_store)
    : event_store_(event_store) {
}

EventStreamProcessor::~EventStreamProcessor() {
    stop_processing();
}

uint64_t EventStreamProcessor::subscribe(EventHandler handler, EventFilter filter) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    Subscription sub;
    sub.id = next_subscription_id_++;
    sub.handler = handler;
    sub.filter = filter;
    
    subscriptions_.push_back(sub);
    
    return sub.id;
}

void EventStreamProcessor::unsubscribe(uint64_t subscription_id) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    subscriptions_.erase(
        std::remove_if(subscriptions_.begin(), subscriptions_.end(),
            [subscription_id](const Subscription& sub) {
                return sub.id == subscription_id;
            }),
        subscriptions_.end()
    );
}

void EventStreamProcessor::start_processing(SequenceID from_sequence) {
    if (processing_) {
        return;
    }
    
    last_processed_ = from_sequence;
    processing_ = true;
    processing_thread_ = std::thread(&EventStreamProcessor::processing_worker, this);
}

void EventStreamProcessor::stop_processing() {
    if (!processing_) {
        return;
    }
    
    processing_ = false;
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }
}

void EventStreamProcessor::process_new_events() {
    if (!event_store_ || !processing_) {
        return;
    }
    
    SequenceID current_sequence = event_store_->get_latest_sequence();
    SequenceID from = last_processed_ + 1;
    
    if (from > current_sequence) {
        return;  // No new events
    }
    
    auto events = event_store_->get_events(from, current_sequence);
    
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    for (const auto& event : events) {
        for (const auto& sub : subscriptions_) {
            // Apply filter if present
            if (sub.filter && !sub.filter(event)) {
                continue;
            }
            
            // Call handler
            sub.handler(event);
        }
        
        events_processed_++;
    }
    
    last_processed_ = current_sequence;
}

void EventStreamProcessor::processing_worker() {
    while (processing_) {
        process_new_events();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // 100Hz
    }
}

EventStreamProcessor::Statistics EventStreamProcessor::get_statistics() const {
    Statistics stats;
    stats.events_processed = events_processed_.load();
    stats.last_processed_sequence = last_processed_.load();
    
    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(subscriptions_mutex_));
        stats.subscriptions_active = subscriptions_.size();
    }
    
    return stats;
}

// ============================================================================
// 4. CQRS Implementation
// ============================================================================

std::string Command::serialize() const {
    std::ostringstream oss;
    oss << static_cast<uint8_t>(type) << ","
        << order_id << ","
        << user_id << ","
        << instrument_id << ","
        << static_cast<uint8_t>(side) << ","
        << price << ","
        << quantity << ","
        << static_cast<uint8_t>(order_type);
    return oss.str();
}

Command Command::deserialize(const std::string& data) {
    Command cmd;
    std::istringstream iss(data);
    std::string token;
    
    std::getline(iss, token, ',');
    cmd.type = static_cast<Type>(std::stoul(token));
    std::getline(iss, token, ',');
    cmd.order_id = std::stoull(token);
    std::getline(iss, token, ',');
    cmd.user_id = std::stoull(token);
    std::getline(iss, token, ',');
    cmd.instrument_id = std::stoul(token);
    std::getline(iss, token, ',');
    cmd.side = static_cast<OrderSide>(std::stoul(token));
    std::getline(iss, token, ',');
    cmd.price = std::stoll(token);
    std::getline(iss, token, ',');
    cmd.quantity = std::stoll(token);
    std::getline(iss, token, ',');
    cmd.order_type = static_cast<OrderType>(std::stoul(token));
    
    return cmd;
}

CommandHandler::CommandHandler(MatchingEngineEventSourcing* engine)
    : engine_(engine) {
}

bool CommandHandler::handle_command(const Command& command, std::vector<Event>& events) {
    if (!validate_command(command)) {
        return false;
    }
    
    switch (command.type) {
        case Command::PLACE_ORDER: {
            Order order(command.order_id, command.user_id, command.instrument_id,
                       command.side, command.price, command.quantity, command.order_type);
            auto trades = engine_->process_order_es(&order);
            
            // Events are automatically published by engine
            // In production, would collect events from event store
            break;
        }
        case Command::CANCEL_ORDER: {
            engine_->cancel_order_es(command.order_id, command.user_id);
            break;
        }
        default:
            return false;
    }
    
    return true;
}

bool CommandHandler::validate_command(const Command& command) const {
    if (command.order_id == 0 || command.user_id == 0 || command.instrument_id == 0) {
        return false;
    }
    if (command.price <= 0 || command.quantity <= 0) {
        return false;
    }
    return true;
}


QueryResult QueryHandler::execute_query(const Query& query) {
    QueryResult result;
    result.success = false;
    
    if (!event_store_) {
        result.error_message = "Event store not initialized";
        return result;
    }
    
    switch (query.type) {
        case Query::GET_EVENTS: {
            result.events = event_store_->get_events(query.from_sequence, query.to_sequence);
            result.success = true;
            break;
        }
        case Query::GET_ORDER: {
            auto events = event_store_->get_order_events(query.order_id);
            result.events = events;
            result.success = true;
            break;
        }
        case Query::GET_USER_ORDERS: {
            // In production, would have user order index
            result.success = true;
            break;
        }
        default:
            result.error_message = "Unsupported query type";
            break;
    }
    
    return result;
}

Order* QueryHandler::get_order(OrderID order_id) {
    // Reconstruct order from events
    auto events = event_store_->get_order_events(order_id);
    
    // Find ORDER_PLACED event
    for (const auto& event : events) {
        if (event.type == EventType::ORDER_PLACED) {
            // Reconstruct order
            // In production, would maintain materialized view
            return nullptr;  // Simplified
        }
    }
    
    return nullptr;
}

void QueryHandler::get_order_book(InstrumentID instrument_id,
                                 std::vector<PriceLevel>& bids,
                                 std::vector<PriceLevel>& asks) {
    // Reconstruct order book from events
    // In production, would maintain materialized view
    auto events = event_store_->get_instrument_events(instrument_id);
    
    // Process events to rebuild order book
    // Simplified - would need full implementation
}

void QueryHandler::update_cache_from_events(const std::vector<Event>& events) {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    for (const auto& event : events) {
        if (event.type == EventType::ORDER_PLACED) {
            Order order(event.data.order_placed.order_id,
                       event.data.order_placed.user_id,
                       event.instrument_id,
                       event.data.order_placed.side,
                       event.data.order_placed.price,
                       event.data.order_placed.quantity,
                       event.data.order_placed.order_type);
            order_cache_[order.order_id] = order;
        }
    }
}

CQRSManager::CQRSManager(MatchingEngineEventSourcing* engine, EventStore* event_store)
    : command_handler_(std::make_unique<CommandHandler>(engine)),
      query_handler_(std::make_unique<QueryHandler>(event_store)),
      stream_processor_(std::make_unique<EventStreamProcessor>(event_store)) {
    
    // Subscribe to events for cache update
    stream_processor_->subscribe(
        [this](const Event& event) {
            cache_update_handler(event);
        }
    );
}

CQRSManager::~CQRSManager() {
    stop_cache_update();
}

bool CQRSManager::execute_command(const Command& command) {
    std::vector<Event> events;
    return command_handler_->handle_command(command, events);
}

QueryResult CQRSManager::execute_query(const Query& query) {
    return query_handler_->execute_query(query);
}

void CQRSManager::start_cache_update() {
    stream_processor_->start_processing();
}

void CQRSManager::stop_cache_update() {
    stream_processor_->stop_processing();
}

void CQRSManager::cache_update_handler(const Event& event) {
    std::vector<Event> events = {event};
    query_handler_->update_cache_from_events(events);
}

// ============================================================================
// 5. Event Versioning Implementation
// ============================================================================

EventVersionManager::EventVersionManager() {
    // Register default schemas for each event type
    for (int i = 0; i <= static_cast<int>(EventType::ORDER_FULLY_FILLED); ++i) {
        EventType type = static_cast<EventType>(i);
        EventSchema schema;
        schema.version = 1;
        schema.schema_name = "v1";
        current_versions_[type] = 1;
        schemas_[type][1] = schema;
    }
}

void EventVersionManager::register_schema(EventType event_type, const EventSchema& schema) {
    std::unique_lock<std::shared_mutex> lock(schema_mutex_);
    schemas_[event_type][schema.version] = schema;
    if (schema.version > current_versions_[event_type]) {
        current_versions_[event_type] = schema.version;
    }
}

EventVersion EventVersionManager::get_current_version(EventType event_type) const {
    std::shared_lock<std::shared_mutex> lock(schema_mutex_);
    auto it = current_versions_.find(event_type);
    if (it != current_versions_.end()) {
        return it->second;
    }
    return 1;  // Default version
}

VersionedEvent EventVersionManager::migrate_to_latest(const VersionedEvent& event) const {
    EventVersion current = get_current_version(event.event.type);
    return migrate_to_version(event, current);
}

VersionedEvent EventVersionManager::migrate_to_version(const VersionedEvent& event, 
                                                     EventVersion target_version) const {
    if (event.version == target_version) {
        return event;  // No migration needed
    }
    
    if (target_version < event.min_supported_version || 
        target_version > event.max_supported_version) {
        // Cannot migrate - return original
        return event;
    }
    
    // Find migration path
    auto path = get_migration_path(event.event.type, event.version, target_version);
    
    VersionedEvent result = event;
    for (EventVersion version : path) {
        // Apply migration
        // In production, would call registered migration function
        result.version = version;
    }
    
    return result;
}

bool EventVersionManager::is_version_supported(EventType event_type, EventVersion version) const {
    std::shared_lock<std::shared_mutex> lock(schema_mutex_);
    auto it = schemas_.find(event_type);
    if (it == schemas_.end()) {
        return false;
    }
    return it->second.find(version) != it->second.end();
}

std::vector<EventVersion> EventVersionManager::get_migration_path(EventType event_type,
                                                                 EventVersion from,
                                                                 EventVersion to) const {
    std::vector<EventVersion> path;
    
    if (from == to) {
        return path;
    }
    
    if (from < to) {
        // Forward migration
        for (EventVersion v = from + 1; v <= to; ++v) {
            path.push_back(v);
        }
    } else {
        // Backward migration
        for (EventVersion v = from - 1; v >= to; --v) {
            path.push_back(v);
        }
    }
    
    return path;
}

void EventVersionManager::register_migration(EventType event_type,
                                            EventVersion from_version,
                                            EventVersion to_version,
                                            MigrationFunc migration) {
    std::unique_lock<std::shared_mutex> lock(schema_mutex_);
    migrations_[event_type][{from_version, to_version}] = migration;
}

std::string VersionedEvent::serialize(EventVersion target_version) const {
    // Serialize with version info
    std::ostringstream oss;
    oss << version << "," << min_supported_version << "," << max_supported_version << ",";
    oss << event.serialize();
    return oss.str();
}

VersionedEvent VersionedEvent::deserialize(const std::string& data, EventVersion version) {
    VersionedEvent ve;
    std::istringstream iss(data);
    std::string token;
    
    std::getline(iss, token, ',');
    ve.version = std::stoul(token);
    std::getline(iss, token, ',');
    ve.min_supported_version = std::stoul(token);
    std::getline(iss, token, ',');
    ve.max_supported_version = std::stoul(token);
    
    // Deserialize event
    std::string event_data = data.substr(data.find(',', data.find(',', data.find(',') + 1) + 1) + 1);
    ve.event = Event::deserialize(event_data);
    
    return ve;
}

VersionedEvent VersionedEvent::migrate_to_version(EventVersion target_version) const {
    // In production, would use EventVersionManager
    VersionedEvent result = *this;
    result.version = target_version;
    return result;
}

bool VersionedEventStore::append_versioned_event(const VersionedEvent& event) {
    // Append as regular event (version info stored in serialization)
    return append_event(event.event);
}

std::vector<VersionedEvent> VersionedEventStore::get_versioned_events(SequenceID from,
                                                                     SequenceID to,
                                                                     EventVersion target_version) const {
    auto events = get_events(from, to);
    std::vector<VersionedEvent> versioned_events;
    
    for (const auto& event : events) {
        VersionedEvent ve;
        ve.event = event;
        ve.version = version_manager_ ? 
            version_manager_->get_current_version(event.type) : 1;
        ve.min_supported_version = 1;
        ve.max_supported_version = ve.version;
        
        // Migrate if needed
        if (target_version > 0 && target_version != ve.version && version_manager_) {
            ve = version_manager_->migrate_to_version(ve, target_version);
        }
        
        versioned_events.push_back(ve);
    }
    
    return versioned_events;
}

VersionedEventStore::VersionedEventStore() : EventStore() {
}

} // namespace perpetual

