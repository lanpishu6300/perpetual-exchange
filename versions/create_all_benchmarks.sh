#!/bin/bash
# 为所有版本创建benchmark的脚本模板

VERSIONS=(
    "original:MatchingEngine:process_order:Red-Black Tree:~300K orders/sec, ~3μs"
    "optimized:OptimizedMatchingEngine:process_order_optimized:Memory Pool + Lock-Free:~300K orders/sec, ~3μs"
    "optimized_v2:MatchingEngineOptimizedV2:process_order_optimized_v2:Hot Path Optimization:~321K orders/sec, ~3μs"
    "art:MatchingEngineART:process_order_art:Adaptive Radix Tree:~410K orders/sec, ~2.3μs"
    "art_simd:MatchingEngineARTSIMD:process_order_art_simd:ART + SIMD:~750K orders/sec, ~1.2μs"
    "event_sourcing:MatchingEngineEventSourcing:process_order_es:Event Sourcing:~300K orders/sec, ~3μs"
    "production_basic:ProductionMatchingEngine:process_order_production:Full Production Features:~15K orders/sec, ~13μs"
)

echo "Benchmark creation script template created"
