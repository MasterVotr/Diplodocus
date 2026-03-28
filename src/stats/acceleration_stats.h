#pragma once

namespace diplodocus {

struct AccelerationStats {
    // Construction statistics
    double build_time = 0.0;
    int node_count = 0;
    int inner_node_count = 0;
    int leaf_node_count = 0;
    int memory_consumption = 0;

    // Query statistics
    int query_count = 0;
};

}  // namespace diplodocus
