#include "typedef.h"
#include <stdlib.h>
#include <string.h>

node_t *root;
symbol_table_t symbol_table;

unsigned int hash(char *name)
{
    unsigned int val = 0, i;
    for (; *name; ++name)
    {
        val = (val << 2) + *name;
        if (i = val & ~TABLE_SIZE)
            val = (val ^ (i >> 12)) & TABLE_SIZE;
    }
    return val;
}

void insert_list(list_head_t *head, list_node_t *node_ptr)
{
    node_ptr->next = head->next;
    head->next = node_ptr;
}

symbol_t *look_up_symbol(char *name)
{
    unsigned int val = hash(name);
    list_head_t *head = &symbol_table[val];
    list_node_t *curr = head->next;
    while (curr)
    {
        if (strcmp(curr->symbol_ptr->name, name) == 0)
            return curr->symbol_ptr;
        curr = curr->next;
    }
    return NULL;
}

void insert_symbol(symbol_t *symbol_ptr)
{
    if (look_up_symbol(symbol_ptr->name))
        return;
    unsigned int val = hash(symbol_ptr->name);
    list_node_t *new_node_ptr = malloc(sizeof(list_node_t));
    new_node_ptr->next = NULL;
    new_node_ptr->symbol_ptr = symbol_ptr;
    insert_list(&symbol_table[val], new_node_ptr);
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
}