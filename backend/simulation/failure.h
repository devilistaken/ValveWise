#ifndef FAILURE_H
#define FAILURE_H

#include "../graph/graph.h"

int fail_pipe(Graph *g, const char *pipe_id);
int restore_pipe(Graph *g, const char *pipe_id);

#endif
