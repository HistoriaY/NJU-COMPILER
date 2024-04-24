#ifndef IR_H
#define IR_H
#include "typedef.h"

typedef struct code_s
{
    char *code_str;
    struct code_s *next;
    struct code_s *prev;
} code_t;
extern code_t *ir_start;

code_t *trans_Cond(node_t *node, char *label_true, char *label_false);
code_t *trans_Exp(node_t *node, char *place);
void trans_Program(node_t *node);
#endif