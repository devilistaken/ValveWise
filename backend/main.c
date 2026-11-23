#include <stdio.h>
#include <stdlib.h>

#include "graph/graph.h"
#include "graph/parser.h"
#include "graph/traversal.h"
#include "simulation/failure.h"

static void report(const Graph *g) {
    ReachabilityResult bfs = graph_bfs(g);
    ReachabilityResult dfs = graph_dfs(g);

    printf("\nNetwork Connectivity Report\n");
    printf("---------------------------\n");
    printf("Connected nodes: %d\n", bfs.connected_count);
    printf("Disconnected nodes: %d\n",
           bfs.disconnected_count);
    printf("Disconnected demand: %.2f\n",
           bfs.disconnected_demand);

    if (bfs.connected_count != dfs.connected_count ||
        bfs.disconnected_count != dfs.disconnected_count) {
        fprintf(stderr, "BFS/DFS mismatch!\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    const char *filename =
        argc >= 2 ? argv[1] : "datasets/Net3.inp";

    Graph *g = parse_epanet(filename);
    if (!g) return EXIT_FAILURE;

    report(g);

    if (argc >= 3) {
        const char *pipe_id = argv[2];

        if (!fail_pipe(g, pipe_id)) {
            fprintf(stderr, "Unknown link: %s\n", pipe_id);
            graph_free(g);
            return EXIT_FAILURE;
        }

        printf("\nSimulated failure: %s\n", pipe_id);
        report(g);
    }

    graph_free(g);
    return EXIT_SUCCESS;
}
