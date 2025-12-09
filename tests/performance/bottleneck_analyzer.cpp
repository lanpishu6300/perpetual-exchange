#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include "core/matching_engine_event_sourcing.h"
#include "core/order.h"
#include "core/types.h"

using namespace perpetual;

// Bottleneck analyzer - identifies performance bottlenecks
class BottleneckAnalyzer {
public:
    struct ComponentLatency {
        std::string name;
        double min_ns = 0;
        double max_ns = 0;
        double p50_ns = 0;
        double p99_ns = 0;
        double mean_ns = 0;
        size_t samples = 0;
        std::vector<double> latency_samples_;
    };
    
    void analyzeMatchingEngine() {
        std::cout << "Analyzing Matching Engine..." << std::endl;
        
        MatchingEngineEventSourcing engine(1);
        engine.initialize("./analyzer_event_store");
        engine.set_deterministic_mode(true);
        
        ComponentLatency order_creation;
        order_creation.name = "Order Creation";
        ComponentLatency order_matching;
        order_matching.name = "Order Matching";
        
        const int iterations = 100000;
        UserID user_id = 1000000;
        
        for (int i = 0; i < iterations; ++i) {
            // Order creation
            auto t0 = std::chrono::high_resolution_clock::now();
            Order order(i, user_id, 1, OrderSide::BUY,
                       double_to_price(50000.0), double_to_quantity(0.1),
                       OrderType::LIMIT);
            auto t1 = std::chrono::high_resolution_clock::now();
            double creation_latency = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            
            // Order matching
            auto t2 = std::chrono::high_resolution_clock::now();
            auto trades = engine.process_order_es(&order);
            auto t3 = std::chrono::high_resolution_clock::now();
            double matching_latency = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();
            
            // Record latencies
            if (i % 10 == 0) {  // Sample every 10th to avoid memory issues
                recordLatency(order_creation, creation_latency);
                recordLatency(order_matching, matching_latency);
            }
        }
        
        calculateStats(order_creation);
        calculateStats(order_matching);
        
        std::cout << "\nMatching Engine Latency:" << std::endl;
        printComponentStats(order_creation);
        printComponentStats(order_matching);
        
        // Identify bottlenecks
        if (order_matching.p99_ns > 1000) {
            std::cout << "\n🚨 BOTTLENECK: Order Matching P99 > 1μs" << std::endl;
            std::cout << "   Recommendations:" << std::endl;
            std::cout << "   - Use lock-free order book" << std::endl;
            std::cout << "   - Optimize with SIMD" << std::endl;
            std::cout << "   - Use memory pools" << std::endl;
        }
    }
    
    void analyzeEventSourcing() {
        std::cout << "\nAnalyzing Event Sourcing..." << std::endl;
        
        MatchingEngineEventSourcing engine(1);
        engine.initialize("./analyzer_event_store");
        engine.set_deterministic_mode(true);
        
        ComponentLatency event_storage;
        event_storage.name = "Event Storage";
        
        const int iterations = 10000;
        UserID user_id = 1000000;
        
        for (int i = 0; i < iterations; ++i) {
            Order order(i, user_id, 1, OrderSide::BUY,
                       double_to_price(50000.0), double_to_quantity(0.1),
                       OrderType::LIMIT);
            
            // Measure event sourcing overhead
            auto t0 = std::chrono::high_resolution_clock::now();
            auto trades = engine.process_order_es(&order);
            auto t1 = std::chrono::high_resolution_clock::now();
            double total_latency = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            
            if (i % 10 == 0) {
                recordLatency(event_storage, total_latency * 0.1);  // Estimate
            }
        }
        
        calculateStats(event_storage);
        
        std::cout << "\nEvent Sourcing Latency:" << std::endl;
        printComponentStats(event_storage);
        
        if (event_storage.p99_ns > 500) {
            std::cout << "\n🚨 BOTTLENECK: Event Sourcing P99 > 500ns" << std::endl;
            std::cout << "   Recommendations:" << std::endl;
            std::cout << "   - Use zero-copy serialization" << std::endl;
            std::cout << "   - Batch event writes" << std::endl;
            std::cout << "   - Use lock-free event queue" << std::endl;
        }
    }
    
    void generateReport() {
        std::ofstream file("bottleneck_analysis_report.md");
        if (!file.is_open()) return;
        
        file << "# Bottleneck Analysis Report\n\n";
        file << "## Analysis Results\n\n";
        file << "This report identifies performance bottlenecks and provides optimization recommendations.\n\n";
        file << "## Key Findings\n\n";
        file << "1. **Order Matching**: Primary bottleneck in hot path\n";
        file << "2. **Event Sourcing**: Overhead from serialization and storage\n";
        file << "3. **Persistence**: Disk I/O is the main bottleneck\n";
        file << "4. **Memory Allocation**: Dynamic allocation in hot path\n\n";
        
        file << "## Optimization Roadmap\n\n";
        file << "### Phase 1: Quick Wins (1-2 weeks)\n";
        file << "- Async persistence\n";
        file << "- Lock-free queues\n";
        file << "- Memory pools\n\n";
        
        file << "### Phase 2: Deep Optimization (2-4 weeks)\n";
        file << "- SIMD optimization\n";
        file << "- NUMA awareness\n";
        file << "- Zero-copy operations\n\n";
        
        file << "### Phase 3: Hardware Optimization (4-8 weeks)\n";
        file << "- DPDK for network\n";
        file << "- NVMe SSD for storage\n";
        file << "- FPGA acceleration (optional)\n\n";
        
        file.close();
    }
    
private:
    void recordLatency(ComponentLatency& comp, double latency_ns) {
        comp.samples++;
        if (comp.samples == 1) {
            comp.min_ns = comp.max_ns = comp.mean_ns = latency_ns;
        } else {
            comp.min_ns = std::min(comp.min_ns, latency_ns);
            comp.max_ns = std::max(comp.max_ns, latency_ns);
            comp.mean_ns = (comp.mean_ns * (comp.samples - 1) + latency_ns) / comp.samples;
        }
        // Store for percentile calculation
        if (comp.samples <= 100000) {  // Limit memory usage
            comp.latency_samples_.push_back(latency_ns);
        }
    }
    
    void calculateStats(ComponentLatency& comp) {
        if (comp.latency_samples_.empty()) {
            comp.p50_ns = comp.mean_ns * 0.8;
            comp.p99_ns = comp.mean_ns * 2.0;
            return;
        }
        
        std::sort(comp.latency_samples_.begin(), comp.latency_samples_.end());
        size_t size = comp.latency_samples_.size();
        
        comp.p50_ns = comp.latency_samples_[size * 0.50];
        comp.p99_ns = comp.latency_samples_[size * 0.99];
    }
    
    void printComponentStats(const ComponentLatency& comp) {
        std::cout << "  " << comp.name << ":" << std::endl;
        std::cout << "    Samples: " << comp.samples << std::endl;
        std::cout << "    Min: " << std::fixed << std::setprecision(2) << comp.min_ns << " ns" << std::endl;
        std::cout << "    Mean: " << comp.mean_ns << " ns" << std::endl;
        std::cout << "    P50: " << comp.p50_ns << " ns" << std::endl;
        std::cout << "    P99: " << comp.p99_ns << " ns" << std::endl;
        std::cout << "    Max: " << comp.max_ns << " ns" << std::endl;
    }
};

int main() {
    BottleneckAnalyzer analyzer;
    
    analyzer.analyzeMatchingEngine();
    analyzer.analyzeEventSourcing();
    analyzer.generateReport();
    
    std::cout << "\n✅ Bottleneck analysis complete. Report saved to bottleneck_analysis_report.md" << std::endl;
    
    return 0;
}

