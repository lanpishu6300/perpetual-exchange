#include "matching_service.h"
#include "core/order.h"
#include "core/types.h"
#include "core/matching_engine_event_sourcing.h"
#include "core/orderbook.h"
#include <grpcpp/grpcpp.h>

namespace perpetual {
namespace matching {

MatchingService::MatchingService(InstrumentID instrument_id, 
                                const std::string& event_store_dir)
    : instrument_id_(instrument_id) {
    matching_engine_ = std::make_unique<MatchingEngineEventSourcing>(instrument_id);
    matching_engine_->initialize(event_store_dir);
    matching_engine_->set_deterministic_mode(true);
}

MatchingService::~MatchingService() {
    stop();
}

bool MatchingService::initialize() {
    return true;
}

void MatchingService::start(const std::string& server_address) {
    if (running_) {
        return;
    }
    
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(this);
    
    grpc_server_ = builder.BuildAndStart();
    if (grpc_server_) {
        running_ = true;
        // Server will run in the calling thread
    }
}

void MatchingService::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    if (grpc_server_) {
        grpc_server_->Shutdown();
    }
}

grpc::Status MatchingService::ProcessOrder(grpc::ServerContext* context,
                                          const OrderRequest* request,
                                          MatchResult* response) {
    try {
        // Convert proto message to Order
        auto order = proto_to_order(*request);
        if (!order) {
            response->set_success(false);
            response->set_error_message("Invalid order request");
            return grpc::Status::OK;
        }
        
        // Process order through matching engine
        auto trades = matching_engine_->process_order_es(order.get());
        
        // Build response
        response->set_success(true);
        for (const auto& trade : trades) {
            Trade* proto_trade = response->add_trades();
            *proto_trade = order_trade_to_proto(trade);
        }
        
        // Update order status
        if (order->is_filled()) {
            response->set_order_status(OrderStatus::ORDER_STATUS_FILLED);
        } else if (order->filled_quantity > 0) {
            response->set_order_status(OrderStatus::ORDER_STATUS_PARTIAL_FILLED);
        } else {
            response->set_order_status(OrderStatus::ORDER_STATUS_PENDING);
        }
        response->set_remaining_quantity(order->remaining_quantity);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_error_message(e.what());
        return grpc::Status::OK;
    }
}

grpc::Status MatchingService::CancelOrder(grpc::ServerContext* context,
                                         const CancelOrderRequest* request,
                                         CancelOrderResponse* response) {
    try {
        bool success = matching_engine_->cancel_order_es(
            request->order_id(), request->user_id());
        
        response->set_success(success);
        if (!success) {
            response->set_error_message("Order not found or cannot be cancelled");
        }
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_error_message(e.what());
        return grpc::Status::OK;
    }
}

grpc::Status MatchingService::GetOrderBook(grpc::ServerContext* context,
                                          const InstrumentRequest* request,
                                          OrderBookSnapshot* response) {
    try {
        if (request->instrument_id() != instrument_id_) {
            return grpc::Status(grpc::INVALID_ARGUMENT, "Invalid instrument ID");
        }
        
        response->set_instrument_id(instrument_id_);
        response->set_timestamp(get_current_timestamp());
        fill_order_book_snapshot(response);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::INTERNAL, e.what());
    }
}

grpc::Status MatchingService::GetBestPrice(grpc::ServerContext* context,
                                          const InstrumentRequest* request,
                                          BestPriceResponse* response) {
    try {
        if (request->instrument_id() != instrument_id_) {
            return grpc::Status(grpc::INVALID_ARGUMENT, "Invalid instrument ID");
        }
        
        const auto& orderbook = matching_engine_->get_orderbook();
        Price best_bid = orderbook.best_bid();
        Price best_ask = orderbook.best_ask();
        
        response->set_best_bid(best_bid);
        response->set_best_ask(best_ask);
        response->set_spread(best_ask > best_bid ? best_ask - best_bid : 0);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::INTERNAL, e.what());
    }
}

std::unique_ptr<Order> MatchingService::proto_to_order(const OrderRequest& request) {
    auto order = std::make_unique<Order>(
        request.order_id(),
        request.user_id(),
        request.instrument_id(),
        request.side() == OrderSide::ORDER_SIDE_BUY ? OrderSide::BUY : OrderSide::SELL,
        request.price(),
        request.quantity(),
        static_cast<OrderType>(request.order_type())
    );
    
    order->sequence_id = request.sequence_id();
    if (order->sequence_id == 0) {
        order->sequence_id = get_current_timestamp();
    }
    order->timestamp = get_current_timestamp();
    
    return order;
}

Trade MatchingService::order_trade_to_proto(const perpetual::Trade& trade) {
    Trade proto_trade;
    proto_trade.set_buy_order_id(trade.buy_order_id);
    proto_trade.set_sell_order_id(trade.sell_order_id);
    proto_trade.set_buy_user_id(trade.buy_user_id);
    proto_trade.set_sell_user_id(trade.sell_user_id);
    proto_trade.set_instrument_id(trade.instrument_id);
    proto_trade.set_price(trade.price);
    proto_trade.set_quantity(trade.quantity);
    proto_trade.set_sequence_id(trade.sequence_id);
    proto_trade.set_is_taker_buy(trade.is_taker_buy);
    return proto_trade;
}

void MatchingService::fill_order_book_snapshot(OrderBookSnapshot* snapshot) {
    const auto& orderbook = matching_engine_->get_orderbook();
    
    // Get depth (top 10 levels)
    std::vector<PriceLevel> bids, asks;
    orderbook.get_depth(10, bids, asks);
    
    // Fill bids
    for (const auto& level : bids) {
        PriceLevel* proto_level = snapshot->add_bids();
        proto_level->set_price(level.price);
        proto_level->set_quantity(level.total_quantity);
        proto_level->set_order_count(1); // Simplified
    }
    
    // Fill asks
    for (const auto& level : asks) {
        PriceLevel* proto_level = snapshot->add_asks();
        proto_level->set_price(level.price);
        proto_level->set_quantity(level.total_quantity);
        proto_level->set_order_count(1); // Simplified
    }
}

} // namespace matching
} // namespace perpetual

