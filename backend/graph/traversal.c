#include "traversal.h"
#include <string.h>

static ReachabilityResult summarize(
    const Graph *g, ReachabilityResult result) {

    for (int i = 0; i < g->node_count; i++) {
        if (result.reachable[i]) {
            result.connected_count++;
        } else {
            result.disconnected_count++;
            result.disconnected_demand += g->nodes[i].demand;
        }
    }

    return result;
}

ReachabilityResult graph_bfs(const Graph *g) {
    ReachabilityResult result = {0};
    if (!g) return result;

    int queue[MAX_NODES];
    int front = 0, rear = 0;

    for (int i = 0; i < g->node_count; i++) {
        if (g->nodes[i].is_source) {
            result.reachable[i] = 1;
            queue[rear++] = i;
        }
    }

    while (front < rear) {
        int u = queue[front++];

        for (Edge *e = g->nodes[u].adj;
             e; e = e->next) {

            if (g->pipes[e->pipe_index].failed)
                continue;

            int v = e->neighbor;

            if (!result.reachable[v]) {
                result.reachable[v] = 1;
                queue[rear++] = v;
            }
        }
    }

    return summarize(g, result);
}

static void dfs_visit(const Graph *g, int u,
                      ReachabilityResult *result) {
    result->reachable[u] = 1;

    for (Edge *e = g->nodes[u].adj;
         e; e = e->next) {

        if (g->pipes[e->pipe_index].failed)
            continue;

        int v = e->neighbor;

        if (!result->reachable[v])
            dfs_visit(g, v, result);
    }
}

ReachabilityResult graph_dfs(const Graph *g) {
    ReachabilityResult result = {0};
    if (!g) return result;

    for (int i = 0; i < g->node_count; i++) {
        if (g->nodes[i].is_source &&
            !result.reachable[i]) {
            dfs_visit(g, i, &result);
        }
    }

    return summarize(g, result);
}
