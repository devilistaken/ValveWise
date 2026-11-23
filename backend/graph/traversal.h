#ifndef TRAVERSAL_H
#define TRAVERSAL_H

#include "graph.h"

typedef struct {
    int reachable[MAX_NODES];
    int connected_count;
    int disconnected_count;
    double disconnected_demand;
} ReachabilityResult;

ReachabilityResult graph_bfs(const Graph *g);
ReachabilityResult graph_dfs(const Graph *g);

#endif
