#include "export.h"
#include "traversal.h"
#include <stdio.h>

static const char *type_name(LinkType type) {
    switch (type) {
        case LINK_PIPE: return "pipe";
        case LINK_PUMP: return "pump";
        case LINK_VALVE: return "valve";
        default: return "unknown";
    }
}

static const char *status_name(LinkStatus status) {
    switch (status) {
        case LINK_OPEN: return "open";
        case LINK_CLOSED: return "closed";
        case LINK_FAILED: return "failed";
        default: return "unknown";
    }
}

static void json_string(FILE *f, const char *s) {
    fputc('"', f);

    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;

        if (c == '"' || c == '\\') {
            fputc('\\', f);
            fputc(c, f);
        } else if (c < 32) {
            fprintf(f, "\\u%04x", c);
        } else {
            fputc(c, f);
        }
    }

    fputc('"', f);
}

int graph_export_json(const Graph *g, const char *filename) {
    if (!g || !filename) return 0;

    FILE *f = fopen(filename, "w");
    if (!f) return 0;

    ReachabilityResult r = graph_bfs(g);

    fprintf(f, "{\n  \"nodes\": [\n");

    for (int i = 0; i < g->node_count; i++) {
        const Node *n = &g->nodes[i];

        fprintf(f, "    {\"id\": ");
        json_string(f, n->id);

        fprintf(f,
            ", \"demand\": %.10g, \"source\": %s,"
            " \"reachable\": %s, \"x\": %.10g,"
            " \"y\": %.10g, \"has_coordinates\": %s}",
            n->demand,
            n->is_source ? "true" : "false",
            r.reachable[i] ? "true" : "false",
            n->x, n->y,
            n->has_coordinates ? "true" : "false"
        );

        fprintf(f, "%s\n",
                i + 1 < g->node_count ? "," : "");
    }

    fprintf(f, "  ],\n  \"links\": [\n");

    for (int i = 0; i < g->pipe_count; i++) {
        const Pipe *p = &g->pipes[i];

        fprintf(f, "    {\"id\": ");
        json_string(f, p->id);

        fprintf(f, ", \"source\": ");
        json_string(f, g->nodes[p->from].id);

        fprintf(f, ", \"target\": ");
        json_string(f, g->nodes[p->to].id);

        fprintf(f, ", \"type\": ");
        json_string(f, type_name(p->type));

        fprintf(f, ", \"status\": ");
        json_string(f, status_name(p->status));

        fprintf(f, ", \"length\": %.10g}",
                p->length);

        fprintf(f, "%s\n",
                i + 1 < g->pipe_count ? "," : "");
    }

    fprintf(f,
        "  ],\n"
        "  \"summary\": {\n"
        "    \"total_nodes\": %d,\n"
        "    \"total_links\": %d,\n"
        "    \"connected_nodes\": %d,\n"
        "    \"disconnected_nodes\": %d,\n"
        "    \"disconnected_demand\": %.10g\n"
        "  }\n}\n",
        g->node_count,
        g->pipe_count,
        r.connected_count,
        r.disconnected_count,
        r.disconnected_demand
    );

    int ok = !ferror(f);
    if (fclose(f) != 0) ok = 0;

    return ok;
}
