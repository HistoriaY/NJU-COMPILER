#ifndef TYPEDEF
#define TYPEDEF

// syntax tree
typedef union token_extra_value
{
    // ID
    char *id;
    // TYPE
    enum type_t
    {
        TYPE_token_INT,
        TYPE_token_FLOAT
    } type;
    // INT
    int int_val;
    // FLOAT
    float float_val;
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
        type_sys_STRUCTURE
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
            char **para_names;
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
    enum
    {
        global_scope,
        local_scope,
        para_scope
    } scope; // symbol scope
} symbol_t;

// list def
typedef struct list_node_s
{
    struct list_node_s *next;
    symbol_t *symbol_ptr;
} list_node_t;

typedef struct list_head_s
{
    list_node_t *next;
} list_head_t;

void insert_list(list_head_t *head, list_node_t *node_ptr);

// symbol table
#define TABLE_SIZE 0x3fff
typedef list_head_t symbol_table_t[TABLE_SIZE];
extern symbol_table_t symbol_table;
unsigned int hash(char *name);
symbol_t *look_up_symbol(char *name);
void insert_symbol(symbol_t *symbol_ptr);

#endif
