#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../backend/graph/graph.h"
#include "../backend/graph/traversal.h"
#include "../backend/simulation/state.h"

static void test_network_states(void) {
    Graph *graph = graph_create();
    NetworkState *state = calloc(1, sizeof(NetworkState));

    assert(graph && state);

    int s = graph_add_node(graph, "S", 0, 1);
    int a = graph_add_node(graph, "A", 10, 0);
    int b = graph_add_node(graph, "B", 20, 0);
    int c = graph_add_node(graph, "C", 30, 0);

    assert(graph_add_pipe(graph, "P1", s, a, 1) >= 0);
    assert(graph_add_pipe(graph, "P2", a, b, 1) >= 0);
    assert(graph_add_pipe(graph, "P3", b, c, 1) >= 0);

    network_state_init(state);

    ReachabilityResult r = graph_bfs_state(graph, state);
    assert(r.connected_count == 4);

    assert(network_state_fail_link(state, graph, "P2"));
    assert(network_state_close_link(state, graph, "P3"));

    r = graph_bfs_state(graph, state);
    assert(r.connected_count == 2);
    assert(r.disconnected_count == 2);
    assert(fabs(r.disconnected_demand - 50) < 1e-8);

    ReachabilityResult d = graph_dfs_state(graph, state);

    assert(d.connected_count == r.connected_count);
    assert(d.disconnected_count == r.disconnected_count);
    assert(fabs(d.disconnected_demand -
                r.disconnected_demand) < 1e-8);

    assert(!network_state_close_link(
        state, graph, "P2"
    ));

    assert(!network_state_fail_link(
        state, graph, "DOES_NOT_EXIST"
    ));

    ReachabilityResult original = graph_bfs(graph);
    assert(original.connected_count == 4);

    network_state_init(state);

    r = graph_bfs_state(graph, state);
    assert(r.connected_count == 4);

    assert(graph_set_link_status(
        graph, "P3", LINK_CLOSED
    ));

    r = graph_bfs_state(graph, state);
    assert(r.connected_count == 3);
    assert(r.disconnected_count == 1);

    assert(graph_set_link_status(
        graph, "P3", LINK_OPEN
    ));

    assert(network_state_close_link(
        state, graph, "P1"
    ));

    r = graph_bfs_state(graph, state);
    assert(r.connected_count == 1);
    assert(r.disconnected_count == 3);
    assert(fabs(r.disconnected_demand - 60) < 1e-8);

    free(state);
    graph_free(graph);
}

int main(void) {
    test_network_states();
    puts("All network-state tests passed!");
    return 0;
}
