#ifndef IR_H
#define IR_H
#include "typedef.h"

typedef struct ir_code_s
{
    char *code_str;
    struct ir_code_s *next;
    struct ir_code_s *prev;
} ir_code_t;
extern ir_code_t *ir_start;

ir_code_t *gen_ir_label_code(const char *const label);

ir_code_t *gen_ir_jmp_code(const char *const label);

char *createFormattedString(const char *format, ...);

ir_code_t *new_empty_code();

ir_code_t *merge_code(int count, ...);

char *new_temp();

char *new_label();

char *get_relop(node_t *node);

char *get_VarDec_name(node_t *node);

ir_code_t *trans_Cond(node_t *node, char *label_true, char *label_false);

ir_code_t *trans_array_access(node_t *node, char *base, type_ptr *base_type);

ir_code_t *trans_array_assign(char *base1, char *base2, type_ptr base_type1, type_ptr base_type2);

ir_code_t *trans_Exp(node_t *node, char *place);

ir_code_t *trans_Dec(node_t *node);

ir_code_t *trans_DecList(node_t *node);

ir_code_t *trans_Def(node_t *node);

ir_code_t *trans_DefList(node_t *node);

ir_code_t *trans_StmtList(node_t *node);

ir_code_t *trans_CompSt(node_t *node);

ir_code_t *trans_Stmt(node_t *node);

ir_code_t *trans_Args(node_t *node, char **arg_list, int pos);

ir_code_t *trans_ParamDec(node_t *node);

ir_code_t *trans_VarList(node_t *node);

ir_code_t *trans_FunDec(node_t *node);

ir_code_t *trans_ExtDef(node_t *node);

void trans_all_ExtDef(node_t *node);

void trans_Program(node_t *root);

#endif