#include "analysis.h"

NetworkAnalysis graph_analyze(
    const Graph *graph,
    const ReachabilityResult *reachability
) {
    NetworkAnalysis analysis = {0};

    if (!graph || !reachability)
        return analysis;

    for (int i = 0; i < graph->node_count; i++) {
        const Node *node = &graph->nodes[i];

        if (node->is_source) {
            analysis.source_count++;
            continue;
        }

        analysis.total_junctions++;
        analysis.total_demand += node->demand;

        if (reachability->reachable[i]) {
            analysis.connected_junctions++;
            analysis.connected_demand += node->demand;
        } else {
            analysis.disconnected_junctions++;
            analysis.disconnected_demand += node->demand;
        }
    }

    if (analysis.total_junctions > 0) {
        analysis.junction_connectivity_percent =
            100.0 * analysis.connected_junctions /
            analysis.total_junctions;
    }

    if (analysis.total_demand > 0) {
        analysis.demand_connectivity_percent =
            100.0 * analysis.connected_demand /
            analysis.total_demand;
    }

    return analysis;
}
