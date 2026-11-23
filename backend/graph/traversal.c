#include <stddef.h>
#include "traversal.h"

static ReachabilityResult summarize(
    const Graph *graph,
    ReachabilityResult result
) {
    for (int i = 0; i < graph->node_count; i++) {
        if (result.reachable[i]) {
            result.connected_count++;
        } else {
            result.disconnected_count++;
            result.disconnected_demand +=
                graph->nodes[i].demand;
        }
    }

    return result;
}

ReachabilityResult graph_bfs_state(
    const Graph *graph,
    const NetworkState *state
) {
    ReachabilityResult result = {0};
    if (!graph) return result;

    int queue[MAX_NODES];
    int front = 0;
    int rear = 0;

    for (int i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].is_source) {
            result.reachable[i] = 1;
            queue[rear++] = i;
        }
    }

    while (front < rear) {
        int current = queue[front++];

        for (Edge *edge = graph->nodes[current].adj;
             edge;
             edge = edge->next) {

            if (!network_state_link_open(
                    graph, state, edge->pipe_index))
                continue;

            int neighbor = edge->neighbor;

            if (!result.reachable[neighbor]) {
                result.reachable[neighbor] = 1;
                queue[rear++] = neighbor;
            }
        }
    }

    return summarize(graph, result);
}

ReachabilityResult graph_dfs_state(
    const Graph *graph,
    const NetworkState *state
) {
    ReachabilityResult result = {0};
    if (!graph) return result;

    int stack[MAX_NODES];
    int top = 0;

    for (int source = 0; source < graph->node_count; source++) {
        if (!graph->nodes[source].is_source ||
            result.reachable[source])
            continue;

        result.reachable[source] = 1;
        stack[top++] = source;

        while (top > 0) {
            int current = stack[--top];

            for (Edge *edge = graph->nodes[current].adj;
                 edge;
                 edge = edge->next) {

                if (!network_state_link_open(
                        graph, state, edge->pipe_index))
                    continue;

                int neighbor = edge->neighbor;

                if (!result.reachable[neighbor]) {
                    result.reachable[neighbor] = 1;
                    stack[top++] = neighbor;
                }
            }
        }
    }

    return summarize(graph, result);
}

ReachabilityResult graph_bfs(const Graph *graph) {
    return graph_bfs_state(graph, NULL);
}

ReachabilityResult graph_dfs(const Graph *graph) {
    return graph_dfs_state(graph, NULL);
}
