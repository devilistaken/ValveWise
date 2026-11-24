#ifndef EXPORT_H
#define EXPORT_H

#include "graph.h"
#include "../simulation/state.h"

int graph_export_json(
    const Graph *graph,
    const char *filename
);

int graph_export_json_state(
    const Graph *graph,
    const NetworkState *state,
    const char *filename
);

#endif
