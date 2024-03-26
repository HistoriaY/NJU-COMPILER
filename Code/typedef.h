#ifndef TYPEDEF
#define TYPEDEF

typedef union token_extra_value
{
    // ID
    char *id;
    // TYPE
    enum type_t
    {
        type_INT,
        type_FLOAT
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
    token_extra_value_t tev;
} node_t;
extern node_t *root;

#endif
