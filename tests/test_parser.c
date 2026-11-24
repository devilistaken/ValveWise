#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../backend/graph/graph.h"
#include "../backend/graph/parser.h"
#include "../backend/graph/traversal.h"
#include "../backend/graph/hash.h"
#include "../backend/simulation/failure.h"

int main(void) {
    HashTable *table = calloc(1, sizeof(HashTable));
    assert(table);

    hash_init(table);
    assert(hash_put(table, "J1", 42));
    assert(hash_get(table, "J1") == 42);
    assert(hash_get(table, "MISSING") == -1);
    assert(!hash_put(table, "J1", 99));
    free(table);

    Graph *g = parse_epanet("datasets/test_network.inp");
    assert(g);

    assert(g->node_count == 4);
    assert(g->pipe_count == 3);

    assert(graph_find_node(g, "J2") >= 0);
    assert(graph_find_pipe(g, "P2") >= 0);

    ReachabilityResult r = graph_bfs(g);
    assert(r.connected_count == 4);
    assert(r.disconnected_count == 0);

    assert(fail_pipe(g, "P2"));

    r = graph_bfs(g);
    assert(r.connected_count == 2);
    assert(r.disconnected_count == 2);
    assert(fabs(r.disconnected_demand - 50.0) < 1e-8);

    ReachabilityResult d = graph_dfs(g);
    assert(d.connected_count == r.connected_count);
    assert(d.disconnected_count == r.disconnected_count);

    assert(restore_pipe(g, "P2"));

    r = graph_bfs(g);
    assert(r.connected_count == 4);

    graph_free(g);

    puts("All EPANET parser and hash tests passed!");
    return 0;
}
