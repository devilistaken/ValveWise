#include <stdio.h>
#include "graph/graph.h"
#include "graph/traversal.h"
#include "simulation/failure.h"

static void report(const Graph *g) {
    ReachabilityResult r = graph_bfs(g);

    printf("\nConnected junctions: %d\n",
           r.connected_count);
    printf("Disconnected junctions: %d\n",
           r.disconnected_count);
    printf("Disconnected demand: %.2f\n",
           r.disconnected_demand);
}

int main(void) {
    Graph *g = graph_create();
    if (!g) return 1;

    int source = graph_add_node(g, "SOURCE", 0, 1);
    int a = graph_add_node(g, "A", 10, 0);
    int b = graph_add_node(g, "B", 20, 0);
    int c = graph_add_node(g, "C", 30, 0);
    int d = graph_add_node(g, "D", 40, 0);

    graph_add_pipe(g, "P1", source, a, 100);
    graph_add_pipe(g, "P2", a, b, 80);
    graph_add_pipe(g, "P3", b, c, 90);
    graph_add_pipe(g, "P4", b, d, 70);

    graph_print(g);

    printf("\nBEFORE FAILURE");
    report(g);

    printf("\nSimulating pipe P2 failure...\n");
    fail_pipe(g, "P2");

    printf("\nAFTER FAILURE");
    report(g);

    graph_free(g);
    return 0;
}
