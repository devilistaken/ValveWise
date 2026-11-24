#ifndef HASH_H
#define HASH_H

#include "graph.h"

void hash_init(HashTable *table);
int hash_get(const HashTable *table, const char *key);
int hash_put(HashTable *table, const char *key, int value);

#endif
