#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph/graph.h"
#include "graph/parser.h"
#include "graph/traversal.h"
#include "graph/export.h"
#include "simulation/failure.h"

int main(int argc, char *argv[]) {
    const char *dataset =
        argc >= 2 ? argv[1] : "datasets/Net3.inp";

    Graph *g = parse_epanet(dataset);
    if (!g) return EXIT_FAILURE;

    if (argc >= 3 && strcmp(argv[2], "-") != 0) {
        if (!fail_pipe(g, argv[2])) {
            fprintf(stderr, "Unknown link: %s\n", argv[2]);
            graph_free(g);
            return EXIT_FAILURE;
        }

        printf("Simulated failure: %s\n", argv[2]);
    }

    ReachabilityResult r = graph_bfs(g);

    printf("Connected nodes: %d\n", r.connected_count);
    printf("Disconnected nodes: %d\n", r.disconnected_count);
    printf("Disconnected demand: %.2f\n",
           r.disconnected_demand);

    const char *output =
        argc >= 4 ? argv[3] : "build/network.json";

    if (!graph_export_json(g, output)) {
        fprintf(stderr, "JSON export failed\n");
        graph_free(g);
        return EXIT_FAILURE;
    }

    printf("Exported network to %s\n", output);

    graph_free(g);
    return EXIT_SUCCESS;
}
