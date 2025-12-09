#pragma once

#include "core/matching_engine.h"
#include "core/matching_engine_event_sourcing.h"
#include "core/event_sourcing.h"
#include <grpcpp/grpcpp.h>
#include "proto/matching.grpc.pb.h"
#include <memory>
#include <thread>
#include <atomic>

namespace perpetual {
namespace matching {

// Matching Service - 撮合服务
// 高内聚：所有撮合逻辑集中在这里
// 低耦合：通过gRPC接口与外部通信
class MatchingService final : public MatchingService::Service {
public:
    MatchingService(InstrumentID instrument_id, 
                   const std::string& event_store_dir = "./event_store");
    ~MatchingService();
    
    // Initialize service
    bool initialize();
    
    // Start service
    void start(const std::string& server_address);
    void stop();
    
    // gRPC service methods
    grpc::Status ProcessOrder(grpc::ServerContext* context,
                             const OrderRequest* request,
                             MatchResult* response) override;
    
    grpc::Status CancelOrder(grpc::ServerContext* context,
                            const CancelOrderRequest* request,
                            CancelOrderResponse* response) override;
    
    grpc::Status GetOrderBook(grpc::ServerContext* context,
                             const InstrumentRequest* request,
                             OrderBookSnapshot* response) override;
    
    grpc::Status GetBestPrice(grpc::ServerContext* context,
                             const InstrumentRequest* request,
                             BestPriceResponse* response) override;
    
private:
    // Convert proto message to Order
    std::unique_ptr<Order> proto_to_order(const OrderRequest& request);
    
    // Convert Trade to proto message
    Trade order_trade_to_proto(const perpetual::Trade& trade);
    
    // Get order book snapshot
    void fill_order_book_snapshot(OrderBookSnapshot* snapshot);
    
    InstrumentID instrument_id_;
    std::unique_ptr<MatchingEngineEventSourcing> matching_engine_;
    std::unique_ptr<grpc::Server> grpc_server_;
    std::atomic<bool> running_{false};
};

} // namespace matching
} // namespace perpetual

