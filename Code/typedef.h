#ifndef TYPEDEF_H
#define TYPEDEF_H
#include "hash_table.h"

// syntax tree
typedef union token_extra_value
{
    // ID
    char *id;
    // TYPE
    enum
    {
        TYPE_token_INT,
        TYPE_token_FLOAT
    } type;
    // INT
    int int_val;
    // FLOAT
    float float_val;
    // RELOP
    enum
    {
        relop_g,
        relop_l,
        relop_e,
        relop_ge,
        relop_le,
        relop_ne
    } relop_type;
} token_extra_value_t;

typedef struct node_s
{
    struct node_s *children[7];
    char *name;
    int first_line;
    int is_terminal;
    int is_lval;
    token_extra_value_t tev;
} node_t;
extern node_t *root;

// type system
typedef struct type_s *type_ptr;
typedef struct struct_field_s *struct_field_ptr;

struct type_s
{
    enum
    {
        type_sys_INT,
        type_sys_FLOAT,
        type_sys_FUNCTION,
        type_sys_ARRAY,
        type_sys_STRUCTURE,
        type_sys_ERROR
    } kind;
    union
    {
        // basic type
        int basic;
        // function type
        struct
        {
            type_ptr return_type;
            int para_num;
            type_ptr *para_types;
            // char **para_names;
            int is_defined;
            int first_declare_line;
        } function;
        // array type
        struct
        {
            int size;
            type_ptr elem_type;
        } array;
        // structure type
        struct
        {
            char *struct_name;
            struct_field_ptr struct_field;
        } structure;
    } u;
    int size; // type size in bytes
};

int same_type(type_ptr t1, type_ptr t2);

struct struct_field_s
{
    type_ptr type;               // field type
    char *name;                  // field name
    struct_field_ptr next_field; // next field
};

// symbol table item def
typedef struct symbol_s
{
    char *name;    // symbol name
    type_ptr type; // symbol type
    // enum
    // {
    //     global_scope,
    //     local_scope,
    //     para_scope
    // } scope; // symbol scope
} symbol_t;

// symbol table
extern HashTable_t *symbol_table;
unsigned int str_hash(char *str);
int str_compare(char *str1, char *str2);
void init_symbol_table();
symbol_t *look_up_symbol(char *name);
void insert_symbol(symbol_t *symbol_ptr);

#endif
