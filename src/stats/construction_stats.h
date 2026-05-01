#pragma once

#include <cstdint>

namespace diplodocus {

constexpr int kIntersectionCost = 2;
constexpr int kAabbTraversalCost = 3;
constexpr float kSobbTraversalCost = 4.5f;

struct ConstructionStats {
    float build_time = 0.0;                 // Total build time in ms
    float init_time = 0.0f;                 // Initialization time in ns
    float memcopy_time = 0.0f;              // CPU-GPU memory operations time in ns
    float morton_construction_time = 0.0f;  // Morton encoding time in ns
    float morton_sort_time = 0.0f;          // Radix sort in ns
    float nn_search_time = 0.0f;            // Cluster nearest neighbor search time in ns
    float match_and_classify_time = 0.0f;   // Cluster classification and nearest neighbor matching time in ns
    float prefix_scan_time = 0.0f;          // Prefix scan time in ns
    float merge_and_compact_time = 0.0f;    // Cluster merging and compaction time in ns
    float sobb_refit_time = 0.0f;           // AABB to SOBB refit time in ns
    float kernel_time = 0.0f;               // Total Kernel execution time (on CPU) in ns
    int64_t node_count = 0;
    int64_t inner_node_count = 0;
    int64_t leaf_node_count = 0;
    float bvh_cost = 0.0f;
    int64_t memory_consumption = 0;
};

}  // namespace diplodocus
