#ifndef GRAPH_H
#define GRAPH_H

#define MAX_NODES 1000
#define MAX_PIPES 5000
#define ID_LEN 64

typedef struct Edge {
    int neighbor;
    int pipe_index;
    struct Edge *next;
} Edge;

typedef struct {
    char id[ID_LEN];
    double demand;
    int is_source;
    Edge *adj;
} Node;

typedef struct {
    char id[ID_LEN];
    int from;
    int to;
    double length;
    int failed;
} Pipe;

typedef struct {
    Node nodes[MAX_NODES];
    Pipe pipes[MAX_PIPES];
    int node_count;
    int pipe_count;
} Graph;

Graph *graph_create(void);
void graph_free(Graph *g);

int graph_add_node(Graph *g, const char *id,
                   double demand, int is_source);

int graph_add_pipe(Graph *g, const char *id,
                   int from, int to, double length);

int graph_find_node(const Graph *g, const char *id);
int graph_find_pipe(const Graph *g, const char *id);

void graph_print(const Graph *g);

#endif
