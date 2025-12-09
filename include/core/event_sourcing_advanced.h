#pragma once

#include "event_sourcing.h"
#include "orderbook.h"  // For PriceLevel
#include "types.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <map>
#include <unordered_map>

namespace perpetual {
    class MatchingEngineEventSourcing;  // Forward declaration

// ============================================================================
// 1. Event Compression (事件压缩)
// ============================================================================

// Event compression strategy
enum class CompressionStrategy {
    NONE = 0,
    SNAPSHOT_ONLY = 1,      // Only create snapshots, keep all events
    SNAPSHOT_AND_DELETE = 2, // Create snapshots and delete old events
    ARCHIVE = 3             // Archive old events to compressed files
};

// Event compressor for periodic compression of old events
class EventCompressor {
public:
    EventCompressor(EventStore* event_store);
    ~EventCompressor();
    
    // Configure compression
    void set_strategy(CompressionStrategy strategy) { strategy_ = strategy; }
    void set_compression_interval(SequenceID interval) { compression_interval_ = interval; }
    void set_retention_sequence(SequenceID retention) { retention_sequence_ = retention; }
    
    // Compress events up to a specific sequence
    // Returns number of events compressed
    size_t compress_events(SequenceID up_to_sequence);
    
    // Create snapshot and optionally delete old events
    bool create_snapshot_and_compress(SequenceID sequence, const std::string& snapshot_path);
    
    // Archive old events to compressed file
    bool archive_events(SequenceID from, SequenceID to, const std::string& archive_path);
    
    // Load archived events
    bool load_archived_events(const std::string& archive_path, std::vector<Event>& events);
    
    // Start background compression thread
    void start_background_compression();
    void stop_background_compression();
    
private:
    void compression_worker();
    
    EventStore* event_store_;
    CompressionStrategy strategy_ = CompressionStrategy::SNAPSHOT_ONLY;
    SequenceID compression_interval_ = 100000;  // Compress every 100k events
    SequenceID retention_sequence_ = 0;       // Keep events after this sequence
    
    std::thread compression_thread_;
    std::atomic<bool> running_{false};
    std::mutex compression_mutex_;
    std::condition_variable compression_cv_;
};

// ============================================================================
// 2. Distributed Event Store (分布式事件存储)
// ============================================================================

// Node ID for distributed storage
using NodeID = uint32_t;

// Distributed event store configuration
struct DistributedEventStoreConfig {
    NodeID node_id;
    std::vector<NodeID> replica_nodes;  // Other nodes for replication
    size_t replication_factor = 3;      // Number of replicas
    bool enable_consensus = true;      // Enable consensus protocol
};

// Distributed event store
class DistributedEventStore {
public:
    DistributedEventStore(const DistributedEventStoreConfig& config);
    ~DistributedEventStore();
    
    // Initialize distributed store
    bool initialize(const std::string& data_dir);
    
    // Append event with replication
    bool append_event(const Event& event);
    
    // Get events (may query multiple nodes)
    std::vector<Event> get_events(SequenceID from, SequenceID to) const;
    
    // Get events from specific node
    std::vector<Event> get_events_from_node(NodeID node_id, SequenceID from, SequenceID to) const;
    
    // Replicate event to other nodes
    bool replicate_event(const Event& event, const std::vector<NodeID>& target_nodes);
    
    // Consensus: get majority consensus on event sequence
    SequenceID get_consensus_sequence() const;
    
    // Check if node is available
    bool is_node_available(NodeID node_id) const;
    
    // Get local event store
    EventStore* get_local_store() { return local_store_.get(); }
    
    // Get data directory (for compression)
    std::string get_data_dir() const;
    
private:
    DistributedEventStoreConfig config_;
    std::unique_ptr<EventStore> local_store_;
    
    // Node availability tracking
    std::unordered_map<NodeID, bool> node_availability_;
    mutable std::shared_mutex node_mutex_;
    
    // Replication queue
    std::queue<Event> replication_queue_;
    std::mutex replication_mutex_;
    std::thread replication_thread_;
    std::atomic<bool> replication_running_{false};
    
    void replication_worker();
};

// ============================================================================
// 3. Event Stream Processing (事件流处理)
// ============================================================================

// Event stream processor for real-time event processing
class EventStreamProcessor {
public:
    using EventHandler = std::function<void(const Event&)>;
    using EventFilter = std::function<bool(const Event&)>;
    
    EventStreamProcessor(EventStore* event_store);
    ~EventStreamProcessor();
    
    // Subscribe to event stream
    // Returns subscription ID
    uint64_t subscribe(EventHandler handler, EventFilter filter = nullptr);
    
    // Unsubscribe
    void unsubscribe(uint64_t subscription_id);
    
    // Start processing events from a sequence
    void start_processing(SequenceID from_sequence = 0);
    void stop_processing();
    
    // Process events in real-time (non-blocking)
    void process_new_events();
    
    // Get processing statistics
    struct Statistics {
        size_t events_processed = 0;
        size_t subscriptions_active = 0;
        SequenceID last_processed_sequence = 0;
    };
    Statistics get_statistics() const;
    
private:
    struct Subscription {
        uint64_t id;
        EventHandler handler;
        EventFilter filter;
    };
    
    void processing_worker();
    
    EventStore* event_store_;
    std::vector<Subscription> subscriptions_;
    std::mutex subscriptions_mutex_;
    
    std::thread processing_thread_;
    std::atomic<bool> processing_{false};
    std::atomic<SequenceID> last_processed_{0};
    std::atomic<size_t> events_processed_{0};
    
    uint64_t next_subscription_id_ = 1;
};

// ============================================================================
// 4. CQRS Integration (命令查询职责分离)
// ============================================================================

// Command (Write side)
struct Command {
    enum Type {
        PLACE_ORDER = 0,
        CANCEL_ORDER = 1,
        UPDATE_ORDER = 2
    };
    
    Type type;
    OrderID order_id;
    UserID user_id;
    InstrumentID instrument_id;
    OrderSide side;
    Price price;
    Quantity quantity;
    OrderType order_type;
    
    // Serialization
    std::string serialize() const;
    static Command deserialize(const std::string& data);
};

// Command handler
class CommandHandler {
public:
    CommandHandler(MatchingEngineEventSourcing* engine);
    
    // Handle command and generate events
    bool handle_command(const Command& command, std::vector<Event>& events);
    
    // Validate command
    bool validate_command(const Command& command) const;
    
private:
    MatchingEngineEventSourcing* engine_;
};

// Query (Read side)
struct Query {
    enum Type {
        GET_ORDER = 0,
        GET_ORDER_BOOK = 1,
        GET_USER_ORDERS = 2,
        GET_TRADES = 3,
        GET_EVENTS = 4
    };
    
    Type type;
    OrderID order_id;
    UserID user_id;
    InstrumentID instrument_id;
    SequenceID from_sequence;
    SequenceID to_sequence;
};

// Query result
struct QueryResult {
    bool success;
    std::string error_message;
    std::vector<Event> events;
    // Add other result types as needed
};

// Query handler (read-only, optimized for queries)
class QueryHandler {
public:
    QueryHandler(EventStore* event_store);
    
    // Execute query
    QueryResult execute_query(const Query& query);
    
    // Get order by ID (from events)
    Order* get_order(OrderID order_id);
    
    // Get order book state (from events)
    void get_order_book(InstrumentID instrument_id, 
                       std::vector<PriceLevel>& bids,
                       std::vector<PriceLevel>& asks);
    
private:
    EventStore* event_store_;
    
    // Read model cache (materialized views)
    std::unordered_map<OrderID, Order> order_cache_;
    std::unordered_map<InstrumentID, std::vector<PriceLevel>> orderbook_cache_;
    mutable std::shared_mutex cache_mutex_;
    
public:
    // Update cache from events (public for implementation)
    void update_cache_from_events(const std::vector<Event>& events);
};

// CQRS Manager
class CQRSManager {
public:
    CQRSManager(MatchingEngineEventSourcing* engine, EventStore* event_store);
    ~CQRSManager();
    
    // Command side
    bool execute_command(const Command& command);
    
    // Query side
    QueryResult execute_query(const Query& query);
    
    // Start background cache update
    void start_cache_update();
    void stop_cache_update();
    
private:
    std::unique_ptr<CommandHandler> command_handler_;
    std::unique_ptr<QueryHandler> query_handler_;
    std::unique_ptr<EventStreamProcessor> stream_processor_;
    
    void cache_update_handler(const Event& event);
};

// ============================================================================
// 5. Event Versioning (事件版本化)
// ============================================================================

// Event version
using EventVersion = uint32_t;

// Event schema version
struct EventSchema {
    EventVersion version;
    std::string schema_name;
    std::map<std::string, std::string> fields;  // Field name -> type
};

// Versioned event (extends Event with version info)
struct VersionedEvent {
    Event event;
    EventVersion version;
    EventVersion min_supported_version;
    EventVersion max_supported_version;
    
    // Version-aware serialization
    std::string serialize(EventVersion target_version) const;
    static VersionedEvent deserialize(const std::string& data, EventVersion version);
    
    // Migrate event to target version
    VersionedEvent migrate_to_version(EventVersion target_version) const;
};

// Event version manager
class EventVersionManager {
public:
    EventVersionManager();
    
    // Register event schema
    void register_schema(EventType event_type, const EventSchema& schema);
    
    // Get current version for event type
    EventVersion get_current_version(EventType event_type) const;
    
    // Migrate event to latest version
    VersionedEvent migrate_to_latest(const VersionedEvent& event) const;
    
    // Migrate event to specific version
    VersionedEvent migrate_to_version(const VersionedEvent& event, EventVersion target_version) const;
    
    // Check if version is supported
    bool is_version_supported(EventType event_type, EventVersion version) const;
    
    // Get migration path
    std::vector<EventVersion> get_migration_path(EventType event_type, 
                                                EventVersion from, 
                                                EventVersion to) const;
    
private:
    // Event type -> version -> schema
    std::unordered_map<EventType, std::map<EventVersion, EventSchema>> schemas_;
    std::unordered_map<EventType, EventVersion> current_versions_;
    mutable std::shared_mutex schema_mutex_;
    
    // Migration functions
    using MigrationFunc = std::function<VersionedEvent(const VersionedEvent&)>;
    std::unordered_map<EventType, std::map<std::pair<EventVersion, EventVersion>, MigrationFunc>> migrations_;
    
    // Register migration function
    void register_migration(EventType event_type, 
                           EventVersion from_version, 
                           EventVersion to_version,
                           MigrationFunc migration);
};

// Versioned event store (extends EventStore with versioning)
class VersionedEventStore : public EventStore {
public:
    VersionedEventStore();
    
    // Append versioned event
    bool append_versioned_event(const VersionedEvent& event);
    
    // Get events with version migration
    std::vector<VersionedEvent> get_versioned_events(SequenceID from, SequenceID to, 
                                                     EventVersion target_version = 0) const;
    
    // Set version manager
    void set_version_manager(EventVersionManager* manager) { version_manager_ = manager; }
    
private:
    EventVersionManager* version_manager_ = nullptr;
};

} // namespace perpetual

