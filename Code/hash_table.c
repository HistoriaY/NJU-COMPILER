#include "hash_table.h"

HashTable_t *hash_table_create(unsigned int (*hash_func)(void *key), int (*key_compare)(void *key1, void *key2))
{
    HashTable_t *table = (HashTable_t *)malloc(sizeof(HashTable_t));
    memset(table->buckets, 0, sizeof(table->buckets));
    table->hash_func = hash_func;
    table->key_compare = key_compare;
    return table;
}

void hash_table_insert(HashTable_t *table, void *key, void *value)
{
    unsigned int index = table->hash_func(key);
    HashNode_t *new_node = (HashNode_t *)malloc(sizeof(HashNode_t));
    new_node->key = key;
    new_node->value = value;
    new_node->next = table->buckets[index];
    table->buckets[index] = new_node;
}

void *hash_table_search(HashTable_t *table, void *key)
{
    unsigned int index = table->hash_func(key);
    HashNode_t *node = table->buckets[index];
    while (node != NULL)
    {
        if (table->key_compare(node->key, key) == 0)
        {
            return node->value;
        }
        node = node->next;
    }
    return NULL;
}