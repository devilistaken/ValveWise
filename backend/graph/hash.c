#include "hash.h"
#include <string.h>

static unsigned long hash_key(const char *key) {
    unsigned long hash = 2166136261UL;

    while (*key) {
        hash ^= (unsigned char)*key++;
        hash *= 16777619UL;
    }

    return hash % HASH_SIZE;
}

void hash_init(HashTable *table) {
    memset(table, 0, sizeof(*table));
}

int hash_get(const HashTable *table, const char *key) {
    if (!table || !key) return -1;

    unsigned long start = hash_key(key);

    for (int i = 0; i < HASH_SIZE; i++) {
        unsigned long pos = (start + i) % HASH_SIZE;

        if (!table->used[pos]) return -1;

        if (strcmp(table->keys[pos], key) == 0)
            return table->values[pos];
    }

    return -1;
}

int hash_put(HashTable *table, const char *key, int value) {
    if (!table || !key || !*key ||
        strlen(key) >= ID_LEN) return 0;

    unsigned long start = hash_key(key);

    for (int i = 0; i < HASH_SIZE; i++) {
        unsigned long pos = (start + i) % HASH_SIZE;

        if (table->used[pos]) {
            if (strcmp(table->keys[pos], key) == 0)
                return 0;
        } else {
            table->used[pos] = 1;
            strcpy(table->keys[pos], key);
            table->values[pos] = value;
            return 1;
        }
    }

    return 0;
}
