#ifndef PERPETUAL_EVENT_SOURCING_ADVANCED_H
#define PERPETUAL_EVENT_SOURCING_ADVANCED_H

#include "core/event_sourcing.h"
#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <queue>
#include <unordered_map>
#include <map>
#include <functional>
#include <string>
#include <shared_mutex>
#include <atomic>

namespace perpetual {

// Forward declarations
class EventStore;
class MatchingEngineEventSourcing;

// Type aliases
using NodeID = uint64_t;
using EventVersion = uint32_t;
using MigrationFunc = std::function<void(void*)>;

// Distributed Event Store Configuration
struct DistributedEventStoreConfig {
    NodeID node_id = 1;
    std::vector<NodeID> replica_nodes;
    uint32_t replication_factor = 1;
    bool enable_consensus = false;
    bool enable_replication = false;
    bool single_node_mode = true;
    
    static DistributedEventStoreConfig create_single_node() {
        DistributedEventStoreConfig config;
        config.single_node_mode = true;
        config.enable_consensus = false;
        config.enable_replication = false;
        config.replication_factor = 1;
        return config;
    }
    
    static DistributedEventStoreConfig create_distributed(NodeID node_id, 
                                                          const std::vector<NodeID>& replicas,
                                                          uint32_t factor) {
        DistributedEventStoreConfig config;
        config.node_id = node_id;
        config.replica_nodes = replicas;
        config.replication_factor = factor;
        config.single_node_mode = false;
        config.enable_consensus = true;
        config.enable_replication = true;
        return config;
    }
};

// Event Compression
class EventCompressor {
public:
    enum class CompressionStrategy {
        SNAPSHOT_ONLY,
        SNAPSHOT_AND_DELETE,
        ARCHIVE
    };
    
    EventCompressor(EventStore* event_store);
    ~EventCompressor();
    
    size_t compress_events(SequenceID up_to_sequence);
    void set_strategy(CompressionStrategy strategy) { strategy_ = strategy; }
    void start_background_compression();
    void stop_background_compression();
    
private:
    bool create_snapshot_and_compress(SequenceID sequence, const std::string& snapshot_path);
    bool archive_events(SequenceID from, SequenceID to, const std::string& archive_path);
    void compression_worker();
    
    EventStore* event_store_;
    CompressionStrategy strategy_ = CompressionStrategy::SNAPSHOT_ONLY;
    std::atomic<bool> running_{false};
    std::thread compression_thread_;
    std::mutex compression_mutex_;
    std::condition_variable compression_cv_;
    SequenceID compression_interval_ = 50000;
    SequenceID retention_sequence_ = 0;
};

// Distributed Event Store
class DistributedEventStore {
public:
    DistributedEventStore(const DistributedEventStoreConfig& config);
    ~DistributedEventStore();
    
    bool initialize(const std::string& data_dir);
    bool append_event(const Event& event);
    std::vector<Event> get_events(SequenceID from, SequenceID to) const;
    std::vector<Event> get_events_from_node(NodeID node_id, SequenceID from, SequenceID to) const;
    bool replicate_event(const Event& event, const std::vector<NodeID>& target_nodes);
    SequenceID get_consensus_sequence() const;
    bool is_node_available(NodeID node_id) const;
    std::string get_data_dir() const;
    EventStore* get_local_store() const { return local_store_.get(); }
    
private:
    void replication_worker();
    
    DistributedEventStoreConfig config_;
    std::unique_ptr<EventStore> local_store_;
    std::unordered_map<NodeID, bool> node_availability_;
    std::queue<Event> replication_queue_;
    std::mutex replication_mutex_;
    std::shared_mutex node_mutex_;
    std::atomic<bool> replication_running_{false};
    std::thread replication_thread_;
};

// Event Stream Processing
using EventHandler = std::function<void(const Event&)>;
using EventFilter = std::function<bool(const Event&)>;

class EventStreamProcessor {
public:
    struct Statistics {
        uint64_t events_processed = 0;
        SequenceID last_processed_sequence = 0;
        size_t subscriptions_active = 0;
    };
    
    EventStreamProcessor(EventStore* event_store);
    ~EventStreamProcessor();
    
    uint64_t subscribe(EventHandler handler, EventFilter filter = nullptr);
    void unsubscribe(uint64_t subscription_id);
    void start_processing(SequenceID from_sequence = 0);
    void stop_processing();
    void process_new_events();
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
    uint64_t next_subscription_id_ = 1;
    std::atomic<SequenceID> last_processed_{0};
    std::atomic<bool> processing_{false};
    std::thread processing_thread_;
    std::atomic<uint64_t> events_processed_{0};
};

// CQRS
struct Command {
    enum Type {
        PLACE_ORDER,
        CANCEL_ORDER,
        MODIFY_ORDER
    };
    
    Type type;
    OrderID order_id;
    UserID user_id;
    InstrumentID instrument_id;
    OrderSide side;
    Price price;
    Quantity quantity;
    OrderType order_type;
    
    std::string serialize() const;
    static Command deserialize(const std::string& data);
};

struct Query {
    enum Type {
        GET_EVENTS,
        GET_ORDER,
        GET_USER_ORDERS,
        GET_ORDER_BOOK
    };
    
    Type type;
    OrderID order_id;
    UserID user_id;
    InstrumentID instrument_id;
    SequenceID from_sequence = 0;
    SequenceID to_sequence = UINT64_MAX;
};

struct QueryResult {
    bool success = false;
    std::string error_message;
    std::vector<Event> events;
    Order* order = nullptr;
};

// PriceLevel is defined in orderbook.h, forward declaration here if needed
// struct PriceLevel;

class CommandHandler {
public:
    CommandHandler(MatchingEngineEventSourcing* engine);
    bool handle_command(const Command& command, std::vector<Event>& events);
    
private:
    bool validate_command(const Command& command) const;
    MatchingEngineEventSourcing* engine_;
};

class QueryHandler {
public:
    QueryHandler(EventStore* event_store);
    QueryResult execute_query(const Query& query);
    Order* get_order(OrderID order_id);
    void get_order_book(InstrumentID instrument_id, 
                       std::vector<struct PriceLevel>& bids,
                       std::vector<struct PriceLevel>& asks);
    
private:
    void update_cache_from_events(const std::vector<Event>& events);
    
    EventStore* event_store_;
    std::unordered_map<OrderID, Order> order_cache_;
    mutable std::shared_mutex cache_mutex_;
};

class CQRSManager {
public:
    CQRSManager(MatchingEngineEventSourcing* engine, EventStore* event_store);
    ~CQRSManager();
    
    bool execute_command(const Command& command);
    QueryResult execute_query(const Query& query);
    void start_cache_update();
    void stop_cache_update();
    
private:
    void cache_update_handler(const Event& event);
    
    std::unique_ptr<CommandHandler> command_handler_;
    std::unique_ptr<QueryHandler> query_handler_;
    std::unique_ptr<EventStreamProcessor> stream_processor_;
};

// Event Versioning
struct EventSchema {
    EventVersion version;
    std::string schema_name;
    // Add more fields as needed
};

struct VersionedEvent {
    Event event;
    EventVersion version = 1;
    EventVersion min_supported_version = 1;
    EventVersion max_supported_version = 1;
    
    std::string serialize(EventVersion target_version = 0) const;
    static VersionedEvent deserialize(const std::string& data, EventVersion version = 1);
    VersionedEvent migrate_to_version(EventVersion target_version) const;
};

class EventVersionManager {
public:
    EventVersionManager();
    void register_schema(EventType event_type, const EventSchema& schema);
    EventVersion get_current_version(EventType event_type) const;
    VersionedEvent migrate_to_latest(const VersionedEvent& event) const;
    VersionedEvent migrate_to_version(const VersionedEvent& event, EventVersion target_version) const;
    bool is_version_supported(EventType event_type, EventVersion version) const;
    std::vector<EventVersion> get_migration_path(EventType event_type, 
                                                  EventVersion from, 
                                                  EventVersion to) const;
    void register_migration(EventType event_type, 
                           EventVersion from_version, 
                           EventVersion to_version,
                           MigrationFunc migration);
    
private:
    std::unordered_map<EventType, std::unordered_map<EventVersion, EventSchema>> schemas_;
    std::unordered_map<EventType, EventVersion> current_versions_;
    std::unordered_map<EventType, std::map<std::pair<EventVersion, EventVersion>, MigrationFunc>> migrations_;
    mutable std::shared_mutex schema_mutex_;
};

class VersionedEventStore : public EventStore {
public:
    VersionedEventStore();
    bool append_versioned_event(const VersionedEvent& event);
    std::vector<VersionedEvent> get_versioned_events(SequenceID from, 
                                                     SequenceID to, 
                                                     EventVersion target_version) const;
    void set_version_manager(EventVersionManager* manager) { version_manager_ = manager; }
    
private:
    EventVersionManager* version_manager_ = nullptr;
};

} // namespace perpetual

#endif // PERPETUAL_EVENT_SOURCING_ADVANCED_H

