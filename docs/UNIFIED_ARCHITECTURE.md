# ValveWise Unified Architecture

## Application
The primary dashboard is frontend/index.html.
The original dashboard remains at frontend/legacy.html.

## Backend
The Python HTTP API invokes compiled C engines.

The unified dashboard uses the optimizer C graph for
network analysis, isolation, valve placement and comparisons.

The original network C engine remains available and tested.

## Data Structures
- Adjacency-list graph
- Hash tables
- Queues and BFS
- DFS and Tarjan bridge detection
- Binary heap
- Greedy and exhaustive search

## Algorithms
- Greedy valve placement
- Random placement
- Topology-based placement
- Exhaustive placement, subject to search limits

## Important limitations
The model estimates topology-based isolation disruption.
It does not simulate hydraulic pressure or water flow.
The isolation model still requires independent validation.

## Execution
Build both C backends and run the Python API server.
The dashboard is available on localhost port 8080.
