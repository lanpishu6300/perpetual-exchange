#include "trading_service.h"
#include "core/order.h"
#include "core/types.h"
#include "core/order_validator.h"
#include "core/account_manager.h"
#include "core/position_manager.h"
#include <grpcpp/grpcpp.h>
#include "proto/matching.grpc.pb.h"
#include "proto/trading.grpc.pb.h"
#include <cmath>

using namespace perpetual::matching;

namespace perpetual {
namespace trading {

TradingService::TradingService(const std::string& matching_service_address)
    : matching_service_address_(matching_service_address) {
    order_validator_ = std::make_unique<OrderValidator>();
    account_manager_ = std::make_unique<AccountBalanceManager>();
    position_manager_ = std::make_unique<PositionManager>();
}

TradingService::~TradingService() {
    stop();
}

bool TradingService::initialize() {
    connect_to_matching_service();
    return true;
}

void TradingService::connect_to_matching_service() {
    matching_channel_ = grpc::CreateChannel(
        matching_service_address_,
        grpc::InsecureChannelCredentials()
    );
    matching_stub_ = MatchingService::NewStub(matching_channel_);
}

void TradingService::start(const std::string& server_address) {
    if (running_) {
        return;
    }
    
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(this);
    
    grpc_server_ = builder.BuildAndStart();
    if (grpc_server_) {
        running_ = true;
    }
}

void TradingService::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    if (grpc_server_) {
        grpc_server_->Shutdown();
    }
}

grpc::Status TradingService::SubmitOrder(grpc::ServerContext* context,
                                        const SubmitOrderRequest* request,
                                        SubmitOrderResponse* response) {
    try {
        // Generate order ID
        uint64_t order_id = next_order_id_++;
        
        // Create order object for validation
        Order order(order_id, request->user_id(), request->instrument_id(),
                   request->side() == OrderSide::ORDER_SIDE_BUY ? OrderSide::BUY : OrderSide::SELL,
                   request->price(), request->quantity(),
                   static_cast<OrderType>(request->order_type()));
        
        // Validate order
        auto validation = order_validator_->validate(&order);
        if (!validation.valid) {
            response->set_success(false);
            response->set_error_message(validation.reason);
            return grpc::Status::OK;
        }
        
        // Check account balance
        // Calculate required margin (simplified - would use actual margin calculation)
        double required_margin = (static_cast<double>(request->price()) / PRICE_SCALE) *
                                 (static_cast<double>(request->quantity()) / QTY_SCALE) / 10.0;
        if (!account_manager_->hasSufficientBalance(request->user_id(), required_margin)) {
            response->set_success(false);
            response->set_error_message("Insufficient margin");
            return grpc::Status::OK;
        }
        
        // Check position limit
        if (!position_manager_->checkPositionLimit(request->user_id(), 
                                                  request->instrument_id(),
                                                  request->quantity(),
                                                  static_cast<OrderSide>(request->side()))) {
            response->set_success(false);
            response->set_error_message("Position limit exceeded");
            return grpc::Status::OK;
        }
        
        // Freeze margin
        if (!account_manager_->freezeBalance(request->user_id(), required_margin)) {
            response->set_success(false);
            response->set_error_message("Failed to freeze balance");
            return grpc::Status::OK;
        }
        
        // Prepare request for matching service
        OrderRequest match_request;
        match_request.set_order_id(order_id);
        match_request.set_user_id(request->user_id());
        match_request.set_instrument_id(request->instrument_id());
        match_request.set_side(request->side());
        match_request.set_order_type(request->order_type());
        match_request.set_price(request->price());
        match_request.set_quantity(request->quantity());
        match_request.set_sequence_id(get_current_timestamp());
        
        // Call matching service
        MatchResult match_result;
        grpc::ClientContext client_context;
        grpc::Status status = matching_stub_->ProcessOrder(
            &client_context, match_request, &match_result);
        
        if (!status.ok()) {
            account_manager_->unfreezeBalance(request->user_id(), required_margin);
            response->set_success(false);
            response->set_error_message("Matching service error: " + status.error_message());
            return grpc::Status::OK;
        }
        
        if (!match_result.success()) {
            account_manager_->unfreezeBalance(request->user_id(), required_margin);
            response->set_success(false);
            response->set_error_message(match_result.error_message());
            return grpc::Status::OK;
        }
        
        // Store order
        {
            std::lock_guard<std::mutex> lock(orders_mutex_);
            OrderData order_data;
            order_data.order_id = order_id;
            order_data.user_id = request->user_id();
            order_data.instrument_id = request->instrument_id();
            order_data.side = request->side();
            order_data.order_type = request->order_type();
            order_data.price = request->price();
            order_data.quantity = request->quantity();
            order_data.filled_quantity = request->quantity() - match_result.remaining_quantity();
            order_data.remaining_quantity = match_result.remaining_quantity();
            order_data.status = match_result.order_status();
            order_data.timestamp = get_current_timestamp();
            
            orders_[order_id] = order_data;
            user_orders_[request->user_id()].push_back(order_id);
        }
        
        // Handle trades
        for (const auto& trade : match_result.trades()) {
            handle_trade_event(trade);
        }
        
        // Update order status
        update_order_from_match(order_id, match_result);
        
        response->set_success(true);
        response->set_order_id(order_id);
        response->set_status(match_result.order_status());
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_error_message(e.what());
        return grpc::Status::OK;
    }
}

grpc::Status TradingService::CancelOrder(grpc::ServerContext* context,
                                        const CancelOrderRequest* request,
                                        CancelOrderResponse* response) {
    try {
        // Find order
        std::lock_guard<std::mutex> lock(orders_mutex_);
        auto it = orders_.find(request->order_id());
        if (it == orders_.end() || it->second.user_id != request->user_id()) {
            response->set_success(false);
            response->set_error_message("Order not found");
            return grpc::Status::OK;
        }
        
        if (it->second.status == static_cast<int32_t>(OrderStatus::ORDER_STATUS_FILLED) || 
            it->second.status == static_cast<int32_t>(OrderStatus::ORDER_STATUS_CANCELLED)) {
            response->set_success(false);
            response->set_error_message("Order cannot be cancelled");
            return grpc::Status::OK;
        }
        
        // Call matching service to cancel
        CancelOrderRequest cancel_request;
        cancel_request.set_order_id(request->order_id());
        cancel_request.set_user_id(request->user_id());
        cancel_request.set_instrument_id(it->second.instrument_id);
        
        CancelOrderResponse cancel_response;
        grpc::ClientContext client_context;
        grpc::Status status = matching_stub_->CancelOrder(
            &client_context, cancel_request, &cancel_response);
        
        if (!status.ok() || !cancel_response.success()) {
            response->set_success(false);
            response->set_error_message("Failed to cancel order");
            return grpc::Status::OK;
        }
        
        // Update order status
        it->second.status = static_cast<int32_t>(OrderStatus::ORDER_STATUS_CANCELLED);
        
        // Unfreeze margin
        double required_margin = (static_cast<double>(it->second.price) / PRICE_SCALE) *
                                 (static_cast<double>(it->second.remaining_quantity) / QTY_SCALE) / 10.0;
        account_manager_->unfreezeBalance(request->user_id(), required_margin);
        
        response->set_success(true);
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_error_message(e.what());
        return grpc::Status::OK;
    }
}

grpc::Status TradingService::QueryOrder(grpc::ServerContext* context,
                                       const QueryOrderRequest* request,
                                       QueryOrderResponse* response) {
    try {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        auto it = orders_.find(request->order_id());
        
        if (it == orders_.end()) {
            response->set_success(false);
            response->set_error_message("Order not found");
            return grpc::Status::OK;
        }
        
        if (request->user_id() != 0 && it->second.user_id != request->user_id()) {
            response->set_success(false);
            response->set_error_message("Order not found");
            return grpc::Status::OK;
        }
        
            OrderInfo* order_info = response->mutable_order();
            order_info->set_order_id(it->second.order_id);
            order_info->set_user_id(it->second.user_id);
            order_info->set_instrument_id(it->second.instrument_id);
            order_info->set_side(static_cast<OrderSide>(it->second.side));
            order_info->set_order_type(static_cast<OrderType>(it->second.order_type));
            order_info->set_price(it->second.price);
            order_info->set_quantity(it->second.quantity);
            order_info->set_filled_quantity(it->second.filled_quantity);
            order_info->set_remaining_quantity(it->second.remaining_quantity);
            order_info->set_status(static_cast<OrderStatus>(it->second.status));
            order_info->set_timestamp(it->second.timestamp);
        
        response->set_success(true);
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_error_message(e.what());
        return grpc::Status::OK;
    }
}

grpc::Status TradingService::QueryUserOrders(grpc::ServerContext* context,
                                            const QueryUserOrdersRequest* request,
                                            QueryUserOrdersResponse* response) {
    try {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        auto user_it = user_orders_.find(request->user_id());
        
        if (user_it == user_orders_.end()) {
            response->set_success(true);
            return grpc::Status::OK;
        }
        
        for (uint64_t order_id : user_it->second) {
            auto order_it = orders_.find(order_id);
            if (order_it == orders_.end()) continue;
            
        const auto& order_data = order_it->second;
        
        // Filter by instrument
        if (request->instrument_id() != 0 && 
            order_data.instrument_id != request->instrument_id()) {
            continue;
        }
        
        // Filter by status (255 means all statuses)
        if (request->status() != 255) {
            if (order_data.status != request->status()) {
                continue;
            }
        }
            
            OrderInfo* order_info = response->add_orders();
            order_info->set_order_id(order_data.order_id);
            order_info->set_user_id(order_data.user_id);
            order_info->set_instrument_id(order_data.instrument_id);
            order_info->set_side(static_cast<OrderSide>(order_data.side));
            order_info->set_order_type(static_cast<OrderType>(order_data.order_type));
            order_info->set_price(order_data.price);
            order_info->set_quantity(order_data.quantity);
            order_info->set_filled_quantity(order_data.filled_quantity);
            order_info->set_remaining_quantity(order_data.remaining_quantity);
            order_info->set_status(static_cast<OrderStatus>(order_data.status));
            order_info->set_timestamp(order_data.timestamp);
        }
        
        response->set_success(true);
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_error_message(e.what());
        return grpc::Status::OK;
    }
}

grpc::Status TradingService::QueryAccount(grpc::ServerContext* context,
                                         const QueryAccountRequest* request,
                                         QueryAccountResponse* response) {
    try {
        auto stats = account_manager_->getAccountStats(request->user_id());
        
        AccountInfo* account = response->mutable_account();
        account->set_user_id(request->user_id());
        account->set_balance(stats.balance);
        account->set_frozen(stats.frozen);
        account->set_available(stats.available);
        account->set_used_margin(stats.used_margin);
        
        response->set_success(true);
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_error_message(e.what());
        return grpc::Status::OK;
    }
}

grpc::Status TradingService::QueryPosition(grpc::ServerContext* context,
                                          const QueryPositionRequest* request,
                                          QueryPositionResponse* response) {
    try {
        // Get position from position manager
        // This is simplified - in production would query actual positions
        Quantity position_size = position_manager_->getPositionSize(
            request->user_id(), request->instrument_id());
        
        if (position_size != 0 || request->instrument_id() == 0) {
            PositionInfo* position = response->add_positions();
            position->set_user_id(request->user_id());
            position->set_instrument_id(request->instrument_id());
            position->set_size(position_size);
            // Other fields would be filled from position manager
        }
        
        response->set_success(true);
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_error_message(e.what());
        return grpc::Status::OK;
    }
}

void TradingService::handle_trade_event(const Trade& trade) {
    // Update account balance based on trades
    // This is simplified - in production would calculate PnL and update balances
    
    // Update positions (simplified - would track actual positions)
    // Positions are tracked internally by position manager
}

void TradingService::update_order_from_match(uint64_t order_id, const MatchResult& match_result) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return;
    }
    
    it->second.filled_quantity = it->second.quantity - match_result.remaining_quantity();
    it->second.remaining_quantity = match_result.remaining_quantity();
    it->second.status = match_result.order_status();
}

} // namespace trading
} // namespace perpetual

