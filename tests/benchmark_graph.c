#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../backend/graph/graph.h"
#include "../backend/graph/traversal.h"

static double elapsed_ms(clock_t start, clock_t end) {
    return 1000.0 * (double)(end - start) /
           CLOCKS_PER_SEC;
}

static void benchmark(int count) {
    Graph *graph = graph_create();
    assert(graph);

    char id[ID_LEN];

    clock_t start = clock();

    for (int i = 0; i < count; i++) {
        snprintf(id, sizeof(id), "J%d", i);

        assert(graph_add_node(
            graph, id, 1.0, i == 0
        ) >= 0);
    }

    for (int i = 1; i < count; i++) {
        snprintf(id, sizeof(id), "P%d", i);

        assert(graph_add_pipe(
            graph, id, i - 1, i, 10
        ) >= 0);
    }

    clock_t built = clock();

    const int repetitions = 1000;

    volatile int total = 0;

    for (int i = 0; i < repetitions; i++) {
        ReachabilityResult r = graph_bfs(graph);
        total += r.connected_count;
    }

    clock_t bfs_end = clock();

    for (int i = 0; i < repetitions; i++) {
        ReachabilityResult r = graph_dfs(graph);
        total += r.connected_count;
    }

    clock_t dfs_end = clock();

    assert(total == 2 * repetitions * count);

    printf("%d,%d,%.4f,%.4f,%.4f\n",
        graph->node_count,
        graph->pipe_count,
        elapsed_ms(start, built),
        elapsed_ms(built, bfs_end) / repetitions,
        elapsed_ms(bfs_end, dfs_end) / repetitions
    );

    graph_free(graph);
}

int main(void) {
    puts("nodes,links,build_ms,bfs_avg_ms,dfs_avg_ms");

    benchmark(100);
    benchmark(500);
    benchmark(1000);
    benchmark(2500);
    benchmark(5000);

    return 0;
}
