#include "graph.h"
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

Graph *graph_create(void) {
    return calloc(1, sizeof(Graph));
}

void graph_free(Graph *g) {
    if (!g) return;

    for (int i = 0; i < g->node_count; i++) {
        Edge *e = g->nodes[i].adj;

        while (e) {
            Edge *next = e->next;
            free(e);
            e = next;
        }
    }

    free(g);
}

int graph_find_node(const Graph *g, const char *id) {
    return g ? hash_get(&g->node_index, id) : -1;
}

int graph_find_pipe(const Graph *g, const char *id) {
    return g ? hash_get(&g->pipe_index, id) : -1;
}

int graph_add_node(Graph *g, const char *id,
                   double demand, int is_source) {
    if (!g || !id || !*id ||
        strlen(id) >= ID_LEN ||
        !isfinite(demand) ||
        g->node_count >= MAX_NODES ||
        graph_find_node(g, id) != -1)
        return -1;

    int index = g->node_count;
    Node *n = &g->nodes[index];

    memset(n, 0, sizeof(*n));
    strcpy(n->id, id);
    n->demand = demand;
    n->is_source = is_source;

    if (!hash_put(&g->node_index, id, index))
        return -1;

    g->node_count++;
    return index;
}

int graph_add_pipe(Graph *g, const char *id,
                   int from, int to, double length) {
    if (!g || !id || !*id ||
        strlen(id) >= ID_LEN ||
        !isfinite(length) ||
        g->pipe_count >= MAX_PIPES ||
        graph_find_pipe(g, id) != -1 ||
        from < 0 || to < 0 ||
        from >= g->node_count ||
        to >= g->node_count ||
        from == to || length <= 0)
        return -1;

    Edge *a = malloc(sizeof(Edge));
    Edge *b = malloc(sizeof(Edge));

    if (!a || !b) {
        free(a);
        free(b);
        return -1;
    }

    int index = g->pipe_count;
    Pipe *p = &g->pipes[index];

    memset(p, 0, sizeof(*p));
    strcpy(p->id, id);
    p->from = from;
    p->to = to;
    p->length = length;
    p->type = LINK_PIPE;
    p->status = LINK_OPEN;

    if (!hash_put(&g->pipe_index, id, index)) {
        free(a);
        free(b);
        return -1;
    }

    a->neighbor = to;
    a->pipe_index = index;
    a->next = g->nodes[from].adj;
    g->nodes[from].adj = a;

    b->neighbor = from;
    b->pipe_index = index;
    b->next = g->nodes[to].adj;
    g->nodes[to].adj = b;

    g->pipe_count++;
    return index;
}

int graph_set_coordinates(Graph *g, const char *id,
                          double x, double y) {
    int i = graph_find_node(g, id);
    if (i < 0 || !isfinite(x) || !isfinite(y)) return 0;

    g->nodes[i].x = x;
    g->nodes[i].y = y;
    g->nodes[i].has_coordinates = 1;
    return 1;
}

int graph_set_link_type(Graph *g, const char *id,
                        LinkType type) {
    int i = graph_find_pipe(g, id);

    if (i < 0 || type < LINK_PIPE || type > LINK_VALVE)
        return 0;

    g->pipes[i].type = type;
    return 1;
}

int graph_set_link_status(Graph *g, const char *id,
                          LinkStatus status) {
    int i = graph_find_pipe(g, id);

    if (i < 0 || status < LINK_OPEN || status > LINK_FAILED)
        return 0;

    g->pipes[i].status = status;
    g->pipes[i].failed = (status == LINK_FAILED);
    return 1;
}

void graph_print(const Graph *g) {
    if (!g) return;

    printf("Network: %d nodes, %d links\n",
           g->node_count, g->pipe_count);

    for (int i = 0; i < g->node_count; i++) {
        printf("%s -> ", g->nodes[i].id);

        for (Edge *e = g->nodes[i].adj; e; e = e->next)
            printf("%s ", g->nodes[e->neighbor].id);

        putchar('\n');
    }
}
