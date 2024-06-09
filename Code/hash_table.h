#ifndef HASH_TABLE_H
#define HASH_TABLE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define HASH_TABLE_SIZE 2003

// hash node
typedef struct HashNode_s
{
    void *key;
    void *value;
    struct HashNode_s *next;
} HashNode_t;

// hash table
typedef struct HashTable_s
{
    HashNode_t *buckets[HASH_TABLE_SIZE];
    unsigned int (*hash_func)(void *key);
    int (*key_compare)(void *key1, void *key2);
} HashTable_t;

// init hash table
HashTable_t *hash_table_create(unsigned int (*hash_func)(void *key), int (*key_compare)(void *key1, void *key2));

// insert element
void hash_table_insert(HashTable_t *table, void *key, void *value);

// search element
void *hash_table_search(HashTable_t *table, void *key);
#endif