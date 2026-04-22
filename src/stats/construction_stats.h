#pragma once

namespace diplodocus {

constexpr int kIntersectionCost = 2;
constexpr int kTraversalCost = 3;

struct ConstructionStats {
    float build_time = 0.0;
    float init_time = 0.0f;
    float memcopy_time = 0.0f;
    float nn_search_time = 0.0f;
    float match_and_classify_time = 0.0f;
    float prefix_scan_time = 0.0f;
    float merge_and_compact_time = 0.0f;
    float kernel_time = 0.0f;
    int node_count = 0;
    int inner_node_count = 0;
    int leaf_node_count = 0;
    float bvh_cost = 0.0f;
    int memory_consumption = 0;
};

}  // namespace diplodocus
