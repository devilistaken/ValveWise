#include "failure.h"

int fail_pipe(Graph *g, const char *pipe_id) {
    int index = graph_find_pipe(g, pipe_id);

    if (index < 0) return 0;

    g->pipes[index].failed = 1;
    return 1;
}

int restore_pipe(Graph *g, const char *pipe_id) {
    int index = graph_find_pipe(g, pipe_id);

    if (index < 0) return 0;

    g->pipes[index].failed = 0;
    return 1;
}
