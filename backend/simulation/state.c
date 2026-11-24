#include "state.h"
#include <string.h>

void network_state_init(NetworkState *state) {
    if (state)
        memset(state, 0, sizeof(*state));
}

int network_state_fail_link(
    NetworkState *state,
    const Graph *graph,
    const char *link_id
) {
    if (!state || !graph || !link_id)
        return 0;

    int index = graph_find_pipe(graph, link_id);
    if (index < 0) return 0;

    state->failed[index] = 1;
    state->closed[index] = 0;
    return 1;
}

int network_state_close_link(
    NetworkState *state,
    const Graph *graph,
    const char *link_id
) {
    if (!state || !graph || !link_id)
        return 0;

    int index = graph_find_pipe(graph, link_id);
    if (index < 0 || state->failed[index])
        return 0;

    state->closed[index] = 1;
    return 1;
}

int network_state_restore_link(
    NetworkState *state,
    const Graph *graph,
    const char *link_id
) {
    if (!state || !graph || !link_id)
        return 0;

    int index = graph_find_pipe(graph, link_id);
    if (index < 0) return 0;

    state->failed[index] = 0;
    state->closed[index] = 0;
    return 1;
}

int network_state_link_open(
    const Graph *graph,
    const NetworkState *state,
    int link_index
) {
    if (!graph ||
        link_index < 0 ||
        link_index >= graph->pipe_count)
        return 0;

    if (graph->pipes[link_index].status != LINK_OPEN)
        return 0;

    if (state &&
        (state->failed[link_index] ||
         state->closed[link_index]))
        return 0;

    return 1;
}
