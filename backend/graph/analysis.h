#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "graph.h"
#include "traversal.h"

typedef struct {
    int total_junctions;
    int connected_junctions;
    int disconnected_junctions;
    int source_count;

    double total_demand;
    double connected_demand;
    double disconnected_demand;

    double junction_connectivity_percent;
    double demand_connectivity_percent;
} NetworkAnalysis;

NetworkAnalysis graph_analyze(
    const Graph *graph,
    const ReachabilityResult *reachability
);

#endif
