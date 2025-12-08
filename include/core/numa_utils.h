#pragma once

#include <thread>
#include <vector>
#include <cstddef>

#ifdef __linux__
#include <numa.h>
#include <numaif.h>
#endif

namespace perpetual {

// NUMA-aware utilities
class NUMAUtils {
public:
    static void bind_thread_to_cpu(int cpu_id) {
#ifdef __linux__
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu_id, &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#elif defined(__APPLE__)
        // macOS doesn't have NUMA, but we can set thread affinity
        thread_affinity_policy_data_t policy;
        policy.affinity_tag = cpu_id;
        thread_policy_set(pthread_mach_thread_np(pthread_self()),
                         THREAD_AFFINITY_POLICY,
                         (thread_policy_t)&policy,
                         THREAD_AFFINITY_POLICY_COUNT);
#endif
    }
    
    static void bind_memory_to_node(void* ptr, size_t size, int node) {
#ifdef __linux__
        if (numa_available() >= 0) {
            numa_tonode_memory(ptr, size, node);
        }
#endif
    }
    
    static int get_current_node() {
#ifdef __linux__
        if (numa_available() >= 0) {
            return numa_node_of_cpu(sched_getcpu());
        }
#endif
        return 0;
    }
    
    static int get_cpu_count() {
        return std::thread::hardware_concurrency();
    }
    
    // Allocate memory on specific NUMA node
    static void* allocate_on_node(size_t size, int node) {
#ifdef __linux__
        if (numa_available() >= 0) {
            return numa_alloc_onnode(size, node);
        }
#endif
        return new char[size];
    }
    
    static void free_on_node(void* ptr, size_t size) {
#ifdef __linux__
        if (numa_available() >= 0) {
            numa_free(ptr, size);
            return;
        }
#endif
        delete[] reinterpret_cast<char*>(ptr);
    }
    
    // Get optimal thread distribution across NUMA nodes
    static std::vector<int> get_optimal_thread_distribution(int num_threads) {
        std::vector<int> distribution;
        int cpu_count = get_cpu_count();
        int cpus_per_thread = cpu_count / num_threads;
        
        for (int i = 0; i < num_threads; ++i) {
            distribution.push_back(i * cpus_per_thread);
        }
        
        return distribution;
    }
};

} // namespace perpetual


