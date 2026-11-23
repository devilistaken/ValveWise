#ifndef HASH_H
#define HASH_H

#include "graph.h"

#define HASH_SIZE 16384

typedef struct {
    char keys[HASH_SIZE][ID_LEN];
    int values[HASH_SIZE];
    unsigned char used[HASH_SIZE];
} HashTable;

void hash_init(HashTable *table);
int hash_get(const HashTable *table, const char *key);
int hash_put(HashTable *table, const char *key, int value);

#endif
