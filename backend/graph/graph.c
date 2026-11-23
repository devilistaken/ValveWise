#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    if (!g || !id) return -1;

    for (int i = 0; i < g->node_count; i++) {
        if (strcmp(g->nodes[i].id, id) == 0)
            return i;
    }

    return -1;
}

int graph_find_pipe(const Graph *g, const char *id) {
    if (!g || !id) return -1;

    for (int i = 0; i < g->pipe_count; i++) {
        if (strcmp(g->pipes[i].id, id) == 0)
            return i;
    }

    return -1;
}

int graph_add_node(Graph *g, const char *id,
                   double demand, int is_source) {
    if (!g || !id || !*id ||
        strlen(id) >= ID_LEN ||
        g->node_count >= MAX_NODES ||
        graph_find_node(g, id) != -1)
        return -1;

    int index = g->node_count++;
    Node *n = &g->nodes[index];

    strcpy(n->id, id);
    n->demand = demand;
    n->is_source = is_source;
    n->adj = NULL;

    return index;
}

int graph_add_pipe(Graph *g, const char *id,
                   int from, int to, double length) {
    if (!g || !id || !*id ||
        strlen(id) >= ID_LEN ||
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

    int index = g->pipe_count++;

    strcpy(g->pipes[index].id, id);
    g->pipes[index].from = from;
    g->pipes[index].to = to;
    g->pipes[index].length = length;
    g->pipes[index].failed = 0;

    a->neighbor = to;
    a->pipe_index = index;
    a->next = g->nodes[from].adj;
    g->nodes[from].adj = a;

    b->neighbor = from;
    b->pipe_index = index;
    b->next = g->nodes[to].adj;
    g->nodes[to].adj = b;

    return index;
}

void graph_print(const Graph *g) {
    if (!g) return;

    printf("\nNetwork: %d junctions, %d pipes\n",
           g->node_count, g->pipe_count);

    for (int i = 0; i < g->node_count; i++) {
        printf("%s -> ", g->nodes[i].id);

        for (Edge *e = g->nodes[i].adj;
             e; e = e->next) {
            printf("%s ", g->nodes[e->neighbor].id);
        }

        printf("\n");
    }
}
