CC = clang
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic

GRAPH = backend/graph/graph.c backend/graph/hash.c
TRAVERSAL = backend/graph/traversal.c backend/simulation/state.c
FAILURE = backend/simulation/failure.c
PARSER = backend/graph/parser.c
EXPORT = backend/graph/export.c

ENGINE = $(GRAPH) $(TRAVERSAL) $(FAILURE) $(PARSER) $(EXPORT)

.PHONY: all test benchmark clean

all: build/valvewise

build:
	mkdir -p build

build/valvewise: backend/main.c $(ENGINE) | build
	$(CC) $(CFLAGS) $^ -o $@

build/test_graph: tests/test_graph.c $(GRAPH) $(TRAVERSAL) $(FAILURE) | build
	$(CC) $(CFLAGS) $^ -o $@

build/test_parser: tests/test_parser.c $(GRAPH) $(TRAVERSAL) $(PARSER) $(FAILURE) | build
	$(CC) $(CFLAGS) $^ -o $@

build/test_state: tests/test_state.c $(GRAPH) $(TRAVERSAL) | build
	$(CC) $(CFLAGS) $^ -o $@

build/benchmark_graph: tests/benchmark_graph.c $(GRAPH) $(TRAVERSAL) | build
	$(CC) $(CFLAGS) $^ -o $@

test: build/valvewise build/test_graph build/test_parser build/test_state
	./build/test_graph
	./build/test_parser
	./build/test_state
	python3 -m unittest discover -s tests -p 'test_api.py' -v

benchmark: build/benchmark_graph
	./build/benchmark_graph

clean:
	rm -rf build
