#ifndef NETWORK_STATE_H
#define NETWORK_STATE_H

#include "../graph/graph.h"

typedef struct {
    unsigned char failed[MAX_PIPES];
    unsigned char closed[MAX_PIPES];
} NetworkState;

void network_state_init(NetworkState *state);

int network_state_fail_link(
    NetworkState *state,
    const Graph *graph,
    const char *link_id
);

int network_state_close_link(
    NetworkState *state,
    const Graph *graph,
    const char *link_id
);

int network_state_restore_link(
    NetworkState *state,
    const Graph *graph,
    const char *link_id
);

int network_state_link_open(
    const Graph *graph,
    const NetworkState *state,
    int link_index
);

#endif
