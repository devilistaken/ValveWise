#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "../backend/graph/graph.h"
#include "../backend/graph/traversal.h"
#include "../backend/simulation/failure.h"

int main(void) {
    Graph *g = graph_create();
    assert(g != NULL);

    int s = graph_add_node(g, "S", 0, 1);
    int a = graph_add_node(g, "A", 10, 0);
    int b = graph_add_node(g, "B", 20, 0);
    int c = graph_add_node(g, "C", 30, 0);

    assert(s == 0 && a == 1 && b == 2 && c == 3);

    assert(graph_add_pipe(g, "P1", s, a, 10) >= 0);
    assert(graph_add_pipe(g, "P2", a, b, 10) >= 0);
    assert(graph_add_pipe(g, "P3", b, c, 10) >= 0);

    assert(graph_find_node(g, "B") == b);
    assert(graph_find_pipe(g, "P2") >= 0);

    assert(graph_add_node(g, "A", 10, 0) == -1);
    assert(graph_add_pipe(g, "P1", a, c, 10) == -1);

    ReachabilityResult r = graph_bfs(g);
    assert(r.connected_count == 4);
    assert(r.disconnected_count == 0);

    assert(fail_pipe(g, "P2") == 1);

    r = graph_bfs(g);
    assert(r.connected_count == 2);
    assert(r.disconnected_count == 2);
    assert(fabs(r.disconnected_demand - 50) < 0.00001);

    ReachabilityResult d = graph_dfs(g);
    assert(d.connected_count == r.connected_count);
    assert(d.disconnected_count == r.disconnected_count);

    assert(restore_pipe(g, "P2") == 1);

    r = graph_bfs(g);
    assert(r.connected_count == 4);

    assert(fail_pipe(g, "INVALID") == 0);

    graph_free(g);

    printf("All graph and failure tests passed!\n");
    return 0;
}
