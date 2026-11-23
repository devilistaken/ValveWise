#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "../backend/graph/graph.h"
#include "../backend/graph/analysis.h"
#include "../backend/graph/traversal.h"
#include "../backend/simulation/state.h"

static void test_redundant_loop(void) {
    Graph *g = graph_create();
    assert(g);

    int s = graph_add_node(g, "S", 0, 1);
    int a = graph_add_node(g, "A", 10, 0);
    int b = graph_add_node(g, "B", 20, 0);
    int c = graph_add_node(g, "C", 30, 0);

    assert(graph_add_pipe(g, "P1", s, a, 10) >= 0);
    assert(graph_add_pipe(g, "P2", a, b, 10) >= 0);
    assert(graph_add_pipe(g, "P3", b, c, 10) >= 0);
    assert(graph_add_pipe(g, "P4", c, s, 10) >= 0);

    NetworkState state;
    network_state_init(&state);

    assert(network_state_fail_link(&state, g, "P2"));

    ReachabilityResult r = graph_bfs_state(g, &state);
    assert(r.connected_count == 4);

    NetworkAnalysis a1 = graph_analyze(g, &r);
    assert(a1.total_junctions == 3);
    assert(a1.source_count == 1);
    assert(a1.disconnected_junctions == 0);
    assert(fabs(a1.total_demand - 60) < 1e-9);

    assert(network_state_fail_link(&state, g, "P4"));

    r = graph_bfs_state(g, &state);
    assert(r.connected_count == 2);
    assert(r.disconnected_count == 2);
    assert(fabs(r.disconnected_demand - 50) < 1e-9);

    graph_free(g);
}

static void test_multiple_sources(void) {
    Graph *g = graph_create();
    assert(g);

    int s1 = graph_add_node(g, "S1", 0, 1);
    int s2 = graph_add_node(g, "S2", 0, 1);
    int a = graph_add_node(g, "A", 10, 0);
    int b = graph_add_node(g, "B", 20, 0);
    int c = graph_add_node(g, "C", 30, 0);

    assert(graph_add_pipe(g, "P1", s1, a, 10) >= 0);
    assert(graph_add_pipe(g, "P2", a, b, 10) >= 0);
    assert(graph_add_pipe(g, "P3", b, c, 10) >= 0);
    assert(graph_add_pipe(g, "P4", c, s2, 10) >= 0);

    NetworkState state;
    network_state_init(&state);

    assert(network_state_fail_link(&state, g, "P2"));

    ReachabilityResult r = graph_bfs_state(g, &state);
    assert(r.connected_count == 5);

    assert(network_state_fail_link(&state, g, "P4"));

    r = graph_bfs_state(g, &state);
    assert(r.connected_count == 3);
    assert(r.disconnected_count == 2);

    NetworkAnalysis a1 = graph_analyze(g, &r);

    assert(a1.source_count == 2);
    assert(a1.total_junctions == 3);
    assert(a1.connected_junctions == 1);
    assert(a1.disconnected_junctions == 2);

    assert(fabs(a1.disconnected_demand - 50) < 1e-9);
    assert(fabs(a1.demand_connectivity_percent -
                (100.0 / 6.0)) < 1e-6);

    graph_free(g);
}

static void test_no_source(void) {
    Graph *g = graph_create();
    assert(g);

    int a = graph_add_node(g, "A", 10, 0);
    int b = graph_add_node(g, "B", 20, 0);

    assert(graph_add_pipe(g, "P1", a, b, 10) >= 0);

    ReachabilityResult r = graph_bfs(g);
    assert(r.connected_count == 0);
    assert(r.disconnected_count == 2);

    NetworkAnalysis analysis = graph_analyze(g, &r);

    assert(analysis.source_count == 0);
    assert(analysis.connected_junctions == 0);
    assert(analysis.disconnected_junctions == 2);
    assert(analysis.demand_connectivity_percent == 0);

    graph_free(g);
}

int main(void) {
    test_redundant_loop();
    test_multiple_sources();
    test_no_source();

    puts("All advanced network analysis tests passed!");
    return 0;
}
