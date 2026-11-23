#include "parser.h"
#include "hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

typedef enum {
    NONE,
    JUNCTIONS,
    RESERVOIRS,
    TANKS,
    PIPES,
    PUMPS,
    VALVES
} Section;

static Section get_section(const char *name) {
    if (!strcmp(name, "[JUNCTIONS]")) return JUNCTIONS;
    if (!strcmp(name, "[RESERVOIRS]")) return RESERVOIRS;
    if (!strcmp(name, "[TANKS]")) return TANKS;
    if (!strcmp(name, "[PIPES]")) return PIPES;
    if (!strcmp(name, "[PUMPS]")) return PUMPS;
    if (!strcmp(name, "[VALVES]")) return VALVES;
    return NONE;
}

static int tokenize(char *line, char *tokens[], int max) {
    char *comment = strchr(line, ';');
    if (comment) *comment = '\0';

    int count = 0;
    char *token = strtok(line, " \t\r\n");

    while (token && count < max) {
        tokens[count++] = token;
        token = strtok(NULL, " \t\r\n");
    }

    return count;
}

static int parse_number(const char *s, double *out) {
    char *end;
    double value = strtod(s, &end);

    if (end == s || *end != '\0' || !isfinite(value))
        return 0;

    *out = value;
    return 1;
}

Graph *parse_epanet(const char *filename) {
    FILE *file = fopen(filename, "r");

    if (!file) {
        perror(filename);
        return NULL;
    }

    Graph *g = graph_create();
    HashTable *nodes = calloc(1, sizeof(HashTable));
    HashTable *links = calloc(1, sizeof(HashTable));

    if (!g || !nodes || !links) {
        fprintf(stderr, "Memory allocation failed\n");
        goto fail;
    }

    hash_init(nodes);
    hash_init(links);

    char line[2048];
    char *t[32];

    for (int pass = 0; pass < 2; pass++) {
        rewind(file);
        Section section = NONE;
        int line_number = 0;

        while (fgets(line, sizeof(line), file)) {
            line_number++;

            if (!strchr(line, '\n') && !feof(file)) {
                fprintf(stderr, "Line too long at %d\n",
                        line_number);
                goto fail;
            }

            char *p = line;

            while (isspace((unsigned char)*p)) p++;

            if (*p == '[') {
                char section_name[128];

                if (sscanf(p, "%127s", section_name) == 1)
                    section = get_section(section_name);

                continue;
            }

            int n = tokenize(p, t, 32);
            if (!n) continue;

            if (pass == 0) {
                int source = 0;
                double demand = 0;

                if (section == JUNCTIONS) {
                    if (n < 2) goto bad_line;

                    if (n >= 3 &&
                        !parse_number(t[2], &demand))
                        goto bad_line;
                } else if (section == RESERVOIRS ||
                           section == TANKS) {
                    if (n < 2) goto bad_line;
                    source = 1;
                } else {
                    continue;
                }

                if (demand < 0) {
                    fprintf(stderr,
                        "Negative demand unsupported at line %d\n",
                        line_number);
                    goto fail;
                }

                if (hash_get(nodes, t[0]) != -1)
                    goto bad_line;

                int index = graph_add_node(
                    g, t[0], demand, source
                );

                if (index < 0 ||
                    !hash_put(nodes, t[0], index))
                    goto bad_line;
            } else {
                if (section != PIPES &&
                    section != PUMPS &&
                    section != VALVES)
                    continue;

                if (n < 3) goto bad_line;

                int from = hash_get(nodes, t[1]);
                int to = hash_get(nodes, t[2]);

                if (from < 0 || to < 0)
                    goto bad_line;

                if (hash_get(links, t[0]) != -1)
                    goto bad_line;

                double length = 1.0;

                if (section == PIPES) {
                    if (n < 6 ||
                        !parse_number(t[3], &length) ||
                        length <= 0)
                        goto bad_line;
                }

                int index = graph_add_pipe(
                    g, t[0], from, to, length
                );

                if (index < 0 ||
                    !hash_put(links, t[0], index))
                    goto bad_line;
            }

            continue;

bad_line:
            fprintf(stderr,
                "Invalid or unsupported network entry at line %d (pass %d)\n",
                line_number, pass + 1);
            goto fail;
        }

        if (ferror(file)) {
            fprintf(stderr, "File read error\n");
            goto fail;
        }
    }

    fclose(file);
    free(nodes);
    free(links);

    printf("Loaded EPANET network: %d nodes, %d links\n",
           g->node_count, g->pipe_count);

    return g;

fail:
    fclose(file);
    free(nodes);
    free(links);
    graph_free(g);
    return NULL;
}
