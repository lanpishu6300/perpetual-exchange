#pragma once

#include "core/event_sourcing_advanced.h"
#include "core/order_validator.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include <grpcpp/grpcpp.h>
#include "proto/trading.grpc.pb.h"
#include "proto/matching.grpc.pb.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>

namespace perpetual {
namespace trading {

// Trading Service with CQRS Design
// Command Query Responsibility Segregation
// 
// Command Side: 处理写操作（提交订单、取消订单）
// Query Side: 处理读操作（查询订单、账户、持仓）
// Materialized Views: 从事件构建的读模型

// Command (Write operations)
struct TradingCommand {
    enum Type {
        SUBMIT_ORDER = 0,
        CANCEL_ORDER = 1,
        UPDATE_ACCOUNT = 2
    };
    
    Type type;
    UserID user_id;
    InstrumentID instrument_id;
    OrderID order_id;
    OrderSide side;
    OrderType order_type;
    Price price;
    Quantity quantity;
    
    std::string serialize() const;
    static TradingCommand deserialize(const std::string& data);
};

// Command Result
struct CommandResult {
    bool success = false;
    std::string error_message;
    OrderID order_id = 0;
    std::vector<Event> events;  // Generated events
};

// Query (Read operations)
struct TradingQuery {
    enum Type {
        GET_ORDER = 0,
        GET_USER_ORDERS = 1,
        GET_ACCOUNT = 2,
        GET_POSITION = 3,
        GET_USER_POSITIONS = 4
    };
    
    Type type;
    UserID user_id;
    OrderID order_id;
    InstrumentID instrument_id;
};

// Materialized Views (Read Models)
struct OrderView {
    OrderID order_id;
    UserID user_id;
    InstrumentID instrument_id;
    OrderSide side;
    OrderType order_type;
    Price price;
    Quantity quantity;
    Quantity filled_quantity;
    Quantity remaining_quantity;
    OrderStatus status;
    int64_t timestamp;
    int64_t updated_at;
};

struct AccountView {
    UserID user_id;
    double balance;
    double frozen;
    double available;
    double used_margin;
    double total_equity;
    int64_t last_updated;
};

struct PositionView {
    UserID user_id;
    InstrumentID instrument_id;
    Quantity size;
    Price entry_price;
    Price mark_price;
    double unrealized_pnl;
    double margin_used;
    int64_t last_updated;
};

// Command Handler (Write side)
class TradingCommandHandler {
public:
    TradingCommandHandler(const std::string& matching_service_address);
    
    // Handle command
    CommandResult handleCommand(const TradingCommand& command);
    
    // Submit order command
    CommandResult handleSubmitOrder(const TradingCommand& command);
    
    // Cancel order command
    CommandResult handleCancelOrder(const TradingCommand& command);
    
    // Set services
    void setOrderValidator(OrderValidator* validator) { order_validator_ = validator; }
    void setAccountManager(AccountBalanceManager* am) { account_manager_ = am; }
    void setPositionManager(PositionManager* pm) { position_manager_ = pm; }
    
private:
    std::string matching_service_address_;
    std::unique_ptr<MatchingService::Stub> matching_stub_;
    std::shared_ptr<grpc::Channel> matching_channel_;
    
    OrderValidator* order_validator_ = nullptr;
    AccountBalanceManager* account_manager_ = nullptr;
    PositionManager* position_manager_ = nullptr;
    
    void connect_to_matching_service();
};

// Query Handler (Read side) - optimized for queries
class TradingQueryHandler {
public:
    TradingQueryHandler(EventStore* event_store);
    
    // Execute query
    template<typename TResult>
    TResult executeQuery(const TradingQuery& query);
    
    // Query order (from materialized view)
    OrderView getOrder(OrderID order_id);
    
    // Query user orders (from materialized view)
    std::vector<OrderView> getUserOrders(UserID user_id, 
                                        InstrumentID instrument_id = 0,
                                        OrderStatus status = static_cast<OrderStatus>(255));
    
    // Query account (from materialized view)
    AccountView getAccount(UserID user_id);
    
    // Query position (from materialized view)
    PositionView getPosition(UserID user_id, InstrumentID instrument_id);
    
    // Query user positions (from materialized view)
    std::vector<PositionView> getUserPositions(UserID user_id);
    
    // Update materialized views from events
    void updateViewsFromEvent(const Event& event);
    
private:
    EventStore* event_store_;
    
    // Materialized views (Read Models)
    std::unordered_map<OrderID, OrderView> order_views_;
    std::unordered_map<UserID, AccountView> account_views_;
    std::unordered_map<UserID, std::unordered_map<InstrumentID, PositionView>> position_views_;
    
    // Indexes for fast lookup
    std::unordered_map<UserID, std::vector<OrderID>> user_order_index_;
    
    mutable std::shared_mutex views_mutex_;
    
    // Rebuild views from events
    void rebuildViewsFromEvents();
    
    // Update order view
    void updateOrderView(const Event& event);
    
    // Update account view
    void updateAccountView(const Event& event);
    
    // Update position view
    void updatePositionView(const Event& event);
};

// Trading Service with CQRS
class TradingServiceCQRS final : public TradingService::Service {
public:
    TradingServiceCQRS(const std::string& matching_service_address,
                      EventStore* event_store = nullptr);
    ~TradingServiceCQRS();
    
    // Initialize service
    bool initialize();
    
    // Start service
    void start(const std::string& server_address);
    void stop();
    
    // gRPC service methods (Command side)
    grpc::Status SubmitOrder(grpc::ServerContext* context,
                            const SubmitOrderRequest* request,
                            SubmitOrderResponse* response) override;
    
    grpc::Status CancelOrder(grpc::ServerContext* context,
                            const CancelOrderRequest* request,
                            CancelOrderResponse* response) override;
    
    // gRPC service methods (Query side)
    grpc::Status QueryOrder(grpc::ServerContext* context,
                           const QueryOrderRequest* request,
                           QueryOrderResponse* response) override;
    
    grpc::Status QueryUserOrders(grpc::ServerContext* context,
                                const QueryUserOrdersRequest* request,
                                QueryUserOrdersResponse* response) override;
    
    grpc::Status QueryAccount(grpc::ServerContext* context,
                             const QueryAccountRequest* request,
                             QueryAccountResponse* response) override;
    
    grpc::Status QueryPosition(grpc::ServerContext* context,
                              const QueryPositionRequest* request,
                              QueryPositionResponse* response) override;
    
    // Start background view update
    void startViewUpdate();
    void stopViewUpdate();
    
private:
    // CQRS components
    std::unique_ptr<TradingCommandHandler> command_handler_;
    std::unique_ptr<TradingQueryHandler> query_handler_;
    std::unique_ptr<EventStreamProcessor> event_processor_;
    
    // Services
    std::unique_ptr<OrderValidator> order_validator_;
    std::unique_ptr<AccountBalanceManager> account_manager_;
    std::unique_ptr<PositionManager> position_manager_;
    
    EventStore* event_store_;
    bool owns_event_store_ = false;
    
    std::unique_ptr<grpc::Server> grpc_server_;
    std::atomic<bool> running_{false};
    
    std::atomic<uint64_t> next_order_id_{1000000};
};

} // namespace trading
} // namespace perpetual

