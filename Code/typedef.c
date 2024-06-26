#include "typedef.h"
#include <stdlib.h>
#include <string.h>
#include "str_func.h"

node_t *root;
HashTable_t *symbol_table;

void init_symbol_table()
{
    symbol_table = hash_table_create(str_hash, str_compare);
}

symbol_t *look_up_symbol(char *name)
{
    return (symbol_t *)hash_table_search(symbol_table, name);
}

void insert_symbol(symbol_t *symbol_ptr)
{
    if (look_up_symbol(symbol_ptr->name))
        return;
    hash_table_insert(symbol_table, symbol_ptr->name, symbol_ptr);
}

int same_type(type_ptr t1, type_ptr t2)
{
    if (t1 == NULL || t2 == NULL || t1->kind == type_sys_ERROR || t2->kind == type_sys_ERROR)
        return 0;
    if (t1->kind != t2->kind)
        return 0;
    if (t1->kind == type_sys_INT || t1->kind == type_sys_FLOAT)
        return 1;
    if (t1->kind == type_sys_ARRAY)
    {
        return same_type(t1->u.array.elem_type, t2->u.array.elem_type);
    }
    if (t1->kind == type_sys_STRUCTURE)
    {
        if (strcmp(t1->u.structure.struct_name, t2->u.structure.struct_name) == 0)
            return 1;
        return 0;
    }
    if (t1->kind == type_sys_FUNCTION)
    {
        // PASS
        return 0;
    }
    exit(1);
}