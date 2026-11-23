#include "failure.h"

int fail_pipe(Graph *g, const char *pipe_id) {
    return graph_set_link_status(g, pipe_id, LINK_FAILED);
}

int restore_pipe(Graph *g, const char *pipe_id) {
    return graph_set_link_status(g, pipe_id, LINK_OPEN);
}
