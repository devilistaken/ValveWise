# ValveWise — Person 1 Validation Report

Generated: 2026-11-23T21:09:43
System: macOS-27.2.1-arm64-arm-64bit-Mach-O

## Scope

C-based adjacency-list graph engine, hash indexing, EPANET subset import, BFS/DFS, independent network states, failure scenarios, demand connectivity and frontend/API integration.

## Controlled test network

| Failed links | Disconnected junctions | Disconnected demand |
|---|---:|---:|
| - | 0 | 0 |
| P2 | 2 | 50 |
| P1,P3 | 3 | 60 |

## EPANET Net3 sampled failure analysis

Scenarios sampled: 30

| Failed pipe | Disconnected junctions | Disconnected demand |
|---|---:|---:|
| 137 | 1 | 42.75 |
| 149 | 2 | 7.2 |
| 20 | 0 | 0 |
| 40 | 0 | 0 |
| 50 | 0 | 0 |
| 60 | 0 | 0 |
| 101 | 0 | 0 |
| 103 | 0 | 0 |
| 105 | 0 | 0 |
| 107 | 0 | 0 |

## C graph benchmark

Build time and mean traversal time, in milliseconds.

```csv
nodes,links,build_ms,bfs_avg_ms,dfs_avg_ms
100,99,0.1990,0.0013,0.0013
500,499,0.0940,0.0050,0.0054
1000,999,0.1680,0.0081,0.0078
2500,2499,0.4300,0.0177,0.0179
5000,4999,0.8500,0.0344,0.0347
```

## Algorithm complexity

- BFS: O(V + E) for one traversal.
- DFS: O(V + E) for one traversal.
- Hash lookup: expected O(1), worst case O(H).
- Scenario-state storage: O(MAX_PIPES).

## Limitations

- Graph connectivity is not hydraulic flow or pressure.
- Isolation feasibility is not established by BFS alone.
- Source and tank behavior is simplified.
- EPANET input support is partial.
- Existing valves are imported as links, not necessarily as pipe-end isolation devices.
- The Net3 comparison uses a sampled set of pipes.
- Benchmark results use synthetic chain graphs and do not measure all possible topologies.

## Team integration

Person 2 can use the Graph, NetworkState and ReachabilityResult interfaces described in `PERSON2_INTEGRATION.md`.
