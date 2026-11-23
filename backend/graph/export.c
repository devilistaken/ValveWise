#include "export.h"
#include "traversal.h"
#include "analysis.h"
#include <stdio.h>

static const char *type_name(LinkType type) {
    switch (type) {
        case LINK_PIPE: return "pipe";
        case LINK_PUMP: return "pump";
        case LINK_VALVE: return "valve";
        default: return "unknown";
    }
}

static const char *status_name(
    const Graph *graph,
    const NetworkState *state,
    int index
) {
    if (state && state->failed[index])
        return "failed";

    if (graph->pipes[index].status == LINK_FAILED)
        return "failed";

    if ((state && state->closed[index]) ||
        graph->pipes[index].status == LINK_CLOSED)
        return "closed";

    return "open";
}

static void json_string(FILE *file, const char *str) {
    fputc('"', file);

    for (; *str; str++) {
        unsigned char c = (unsigned char)*str;

        if (c == '"' || c == '\\') {
            fputc('\\', file);
            fputc(c, file);
        } else if (c < 32) {
            fprintf(file, "\\u%04x", c);
        } else {
            fputc(c, file);
        }
    }

    fputc('"', file);
}

int graph_export_json_state(
    const Graph *graph,
    const NetworkState *state,
    const char *filename
) {
    if (!graph || !filename) return 0;

    FILE *file = fopen(filename, "w");
    if (!file) return 0;

    ReachabilityResult result = graph_bfs_state(
        graph, state
    );

    fprintf(file, "{\n  \"nodes\": [\n");

    for (int i = 0; i < graph->node_count; i++) {
        const Node *node = &graph->nodes[i];

        fprintf(file, "    {\"id\": ");
        json_string(file, node->id);

        fprintf(file,
            ", \"demand\": %.10g,"
            " \"source\": %s,"
            " \"reachable\": %s,"
            " \"x\": %.10g,"
            " \"y\": %.10g,"
            " \"has_coordinates\": %s}",
            node->demand,
            node->is_source ? "true" : "false",
            result.reachable[i] ? "true" : "false",
            node->x,
            node->y,
            node->has_coordinates ? "true" : "false"
        );

        fprintf(file, "%s\n",
            i + 1 < graph->node_count ? "," : "");
    }

    fprintf(file, "  ],\n  \"links\": [\n");

    for (int i = 0; i < graph->pipe_count; i++) {
        const Pipe *link = &graph->pipes[i];

        fprintf(file, "    {\"id\": ");
        json_string(file, link->id);

        fprintf(file, ", \"source\": ");
        json_string(file, graph->nodes[link->from].id);

        fprintf(file, ", \"target\": ");
        json_string(file, graph->nodes[link->to].id);

        fprintf(file, ", \"type\": ");
        json_string(file, type_name(link->type));

        fprintf(file, ", \"status\": ");
        json_string(file, status_name(graph, state, i));

        fprintf(file, ", \"length\": %.10g}",
            link->length);

        fprintf(file, "%s\n",
            i + 1 < graph->pipe_count ? "," : "");
    }

    NetworkAnalysis analysis =
        graph_analyze(graph, &result);

    fprintf(file,
        "  ],\n"
        "  \"analysis\": {\n"
        "    \"total_junctions\": %d,\n"
        "    \"connected_junctions\": %d,\n"
        "    \"disconnected_junctions\": %d,\n"
        "    \"source_count\": %d,\n"
        "    \"total_demand\": %.10g,\n"
        "    \"connected_demand\": %.10g,\n"
        "    \"disconnected_demand\": %.10g,\n"
        "    \"junction_connectivity_percent\": %.6f,\n"
        "    \"demand_connectivity_percent\": %.6f\n"
        "  },\n"
        "  \"summary\": {\n"
        "    \"total_nodes\": %d,\n"
        "    \"total_links\": %d,\n"
        "    \"connected_nodes\": %d,\n"
        "    \"disconnected_nodes\": %d,\n"
        "    \"disconnected_demand\": %.10g\n"
        "  }\n}\n",
        analysis.total_junctions,
        analysis.connected_junctions,
        analysis.disconnected_junctions,
        analysis.source_count,
        analysis.total_demand,
        analysis.connected_demand,
        analysis.disconnected_demand,
        analysis.junction_connectivity_percent,
        analysis.demand_connectivity_percent,
        graph->node_count,
        graph->pipe_count,
        result.connected_count,
        result.disconnected_count,
        result.disconnected_demand
    );

    int success = !ferror(file);
    if (fclose(file) != 0)
        success = 0;

    return success;
}

int graph_export_json(
    const Graph *graph,
    const char *filename
) {
    return graph_export_json_state(graph, NULL, filename);
}
