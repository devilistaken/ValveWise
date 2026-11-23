#ifndef GRAPH_H
#define GRAPH_H

#define MAX_NODES 5000
#define MAX_PIPES 20000
#define ID_LEN 64
#define HASH_SIZE 32768

typedef enum {
    LINK_PIPE,
    LINK_PUMP,
    LINK_VALVE
} LinkType;

typedef enum {
    LINK_OPEN,
    LINK_CLOSED,
    LINK_FAILED
} LinkStatus;

typedef struct Edge {
    int neighbor;
    int pipe_index;
    struct Edge *next;
} Edge;

typedef struct {
    char id[ID_LEN];
    double demand;
    int is_source;
    double x, y;
    int has_coordinates;
    Edge *adj;
} Node;

typedef struct {
    char id[ID_LEN];
    int from, to;
    double length;
    int failed;
    LinkType type;
    LinkStatus status;
} Pipe;

typedef struct {
    char keys[HASH_SIZE][ID_LEN];
    int values[HASH_SIZE];
    unsigned char used[HASH_SIZE];
} HashTable;

typedef struct {
    Node nodes[MAX_NODES];
    Pipe pipes[MAX_PIPES];
    int node_count, pipe_count;
    HashTable node_index;
    HashTable pipe_index;
} Graph;

Graph *graph_create(void);
void graph_free(Graph *g);

int graph_add_node(Graph *g, const char *id,
                   double demand, int is_source);

int graph_add_pipe(Graph *g, const char *id,
                   int from, int to, double length);

int graph_find_node(const Graph *g, const char *id);
int graph_find_pipe(const Graph *g, const char *id);

int graph_set_coordinates(Graph *g, const char *id,
                          double x, double y);

int graph_set_link_type(Graph *g, const char *id,
                        LinkType type);

int graph_set_link_status(Graph *g, const char *id,
                          LinkStatus status);

void graph_print(const Graph *g);

#endif
