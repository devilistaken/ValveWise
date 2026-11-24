#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph/graph.h"
#include "graph/parser.h"
#include "graph/traversal.h"
#include "graph/export.h"
#include "simulation/state.h"

static int apply_links(
    NetworkState *state,
    const Graph *graph,
    const char *ids,
    int close_links
) {
    if (!ids || !*ids || strcmp(ids, "-") == 0)
        return 1;

    size_t length = strlen(ids);
    char *copy = malloc(length + 1);

    if (!copy) return 0;

    memcpy(copy, ids, length + 1);

    char *cursor = copy;

    while (*cursor) {
        char *separator = strchr(cursor, ',');

        if (separator)
            *separator = '\0';

        if (!*cursor) {
            free(copy);
            return 0;
        }

        int success = close_links
            ? network_state_close_link(
                state, graph, cursor)
            : network_state_fail_link(
                state, graph, cursor);

        if (!success) {
            fprintf(stderr,
                "Invalid or conflicting link: %s\n",
                cursor);
            free(copy);
            return 0;
        }

        if (!separator)
            break;

        cursor = separator + 1;
    }

    free(copy);
    return 1;
}

int main(int argc, char *argv[]) {
    const char *dataset =
        argc > 1 ? argv[1] : "datasets/Net3.inp";

    const char *failures =
        argc > 2 ? argv[2] : "-";

    const char *output =
        argc > 3 ? argv[3] : "build/network.json";

    const char *closures =
        argc > 4 ? argv[4] : "-";

    Graph *graph = parse_epanet(dataset);
    if (!graph) return EXIT_FAILURE;

    NetworkState *state = calloc(1, sizeof(NetworkState));
    if (!state) {
        graph_free(graph);
        return EXIT_FAILURE;
    }

    network_state_init(state);

    if (!apply_links(state, graph, failures, 0) ||
        !apply_links(state, graph, closures, 1)) {
        free(state);
        graph_free(graph);
        return EXIT_FAILURE;
    }

    ReachabilityResult bfs =
        graph_bfs_state(graph, state);

    ReachabilityResult dfs =
        graph_dfs_state(graph, state);

    if (bfs.connected_count != dfs.connected_count ||
        bfs.disconnected_count != dfs.disconnected_count) {
        fprintf(stderr, "BFS/DFS consistency failure\n");
        free(state);
        graph_free(graph);
        return EXIT_FAILURE;
    }

    printf("Connected nodes: %d\n",
        bfs.connected_count);

    printf("Disconnected nodes: %d\n",
        bfs.disconnected_count);

    printf("Disconnected demand: %.2f\n",
        bfs.disconnected_demand);

    if (!graph_export_json_state(
            graph, state, output)) {
        fprintf(stderr, "JSON export failed\n");
        free(state);
        graph_free(graph);
        return EXIT_FAILURE;
    }

    printf("Exported to %s\n", output);

    free(state);
    graph_free(graph);
    return EXIT_SUCCESS;
}
