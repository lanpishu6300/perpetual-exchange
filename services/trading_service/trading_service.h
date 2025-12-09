#pragma once

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

namespace perpetual {
namespace trading {

// Trading Service - 交易服务
// 高内聚：订单管理、账户管理、持仓管理集中在这里
// 低耦合：通过gRPC调用Matching Service
class TradingService final : public TradingService::Service {
public:
    TradingService(const std::string& matching_service_address);
    ~TradingService();
    
    // Initialize service
    bool initialize();
    
    // Start service
    void start(const std::string& server_address);
    void stop();
    
    // gRPC service methods
    grpc::Status SubmitOrder(grpc::ServerContext* context,
                            const SubmitOrderRequest* request,
                            SubmitOrderResponse* response) override;
    
    grpc::Status CancelOrder(grpc::ServerContext* context,
                            const CancelOrderRequest* request,
                            CancelOrderResponse* response) override;
    
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
    
private:
    // Connect to matching service
    void connect_to_matching_service();
    
    // Handle trade callback from matching service
    void handle_trade_event(const Trade& trade);
    
    // Update order from match result
    void update_order_from_match(uint64_t order_id, const MatchResult& match_result);
    
    std::string matching_service_address_;
    std::unique_ptr<MatchingService::Stub> matching_stub_;
    std::shared_ptr<grpc::Channel> matching_channel_;
    
    // Order storage
    struct OrderData {
        uint64_t order_id;
        uint64_t user_id;
        uint32_t instrument_id;
        int32_t side;  // OrderSide from proto
        int32_t order_type;  // OrderType from proto
        int64_t price;
        int64_t quantity;
        int64_t filled_quantity;
        int64_t remaining_quantity;
        int32_t status;  // OrderStatus from proto
        uint64_t timestamp;
    };
    
    std::unordered_map<uint64_t, OrderData> orders_;
    std::unordered_map<uint64_t, std::vector<uint64_t>> user_orders_;
    mutable std::mutex orders_mutex_;
    
    // Services
    std::unique_ptr<perpetual::OrderValidator> order_validator_;
    std::unique_ptr<perpetual::AccountBalanceManager> account_manager_;
    std::unique_ptr<perpetual::PositionManager> position_manager_;
    
    std::unique_ptr<grpc::Server> grpc_server_;
    std::atomic<bool> running_{false};
    
    // Order ID generator
    std::atomic<uint64_t> next_order_id_{1000000};
};

} // namespace trading
} // namespace perpetual

