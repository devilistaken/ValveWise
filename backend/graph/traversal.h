#ifndef TRAVERSAL_H
#define TRAVERSAL_H

#include "graph.h"
#include "../simulation/state.h"

typedef struct {
    int reachable[MAX_NODES];
    int connected_count;
    int disconnected_count;
    double disconnected_demand;
} ReachabilityResult;

ReachabilityResult graph_bfs(const Graph *graph);
ReachabilityResult graph_dfs(const Graph *graph);

ReachabilityResult graph_bfs_state(
    const Graph *graph,
    const NetworkState *state
);

ReachabilityResult graph_dfs_state(
    const Graph *graph,
    const NetworkState *state
);

#endif
