#ifndef SEMANTICS_H
#define SEMANTICS_H
#include "typedef.h"

typedef struct VarDec_info_s
{
    // VarDec: ID
    // VarDec: VarDec LB INT RB
    char *id;
    type_ptr type;
    node_t *VarDec_node;
} VarDec_info_t;

// void free_VarDec_info(VarDec_info_t *info);

typedef struct Dec_info_s
{
    // Dec: VarDec | VarDec ASSIGNOP Exp
    VarDec_info_t var_dec_info;
    node_t *assign_Exp;
    node_t *Dec_node;
} Dec_info_t;

// void free_Dec_info(Dec_info_t *info);

typedef struct DecList_info_s
{
    // DecList: Dec | Dec COMMA DecList
    int dec_info_num;
    Dec_info_t *dec_infos;
    node_t *DecList_node;
} DecList_info_t;

void free_DecList_info(DecList_info_t *info);

typedef struct Def_info_s
{
    // Def: Specifier DecList SEMI
    type_ptr base_type;
    DecList_info_t dec_list_info;
    node_t *Def_node;
} Def_info_t;

void free_Def_info(Def_info_t *info);

typedef struct DefList_info_s
{
    // DefList: Def DefList | empty
    int def_info_num;
    Def_info_t *def_infos;
    node_t *DefList_node;
} DefList_info_t;

void free_DefList_info(DefList_info_t *info);

// typedef struct ExtDecList_info_s
// {
//     // ExtDecList: VarDec | VarDec COMMA ExtDecList
//     int var_dec_info_num;
//     VarDec_info_t *var_dec_infos;
//     node_t *ExtDecList_node;
// } ExtDecList_info_t;

typedef struct VarList_info_s
{
    // VarList: ParamDec COMMA VarList | ParamDec
    int var_dec_num;
    VarDec_info_t *var_dec_infos;
    node_t *VarList_node;
} VarList_info_t;

void free_VarList_info(VarList_info_t *info);

void init_basic_type_ptr();

void init_built_in_func();

void semantic_error_print(int error_type, int line, char *msg);

char *next_anony_struct_name();

type_ptr deal_StructSpecifier(node_t *node);

type_ptr deal_Specifier(node_t *node);

void deal_ExtDecList(node_t *node, type_ptr base_type);

VarList_info_t deal_VarList(node_t *node, int prev_para_num);

int deal_FunDec(node_t *node, type_ptr return_type, int is_definition);

void deal_Stmt(node_t *node, type_ptr return_type);

void deal_StmtList(node_t *node, type_ptr return_type);

void deal_CompSt(node_t *node, type_ptr return_type);

void deal_ExtDef(node_t *node);

VarDec_info_t deal_VarDec(node_t *node, type_ptr base_type);

Dec_info_t deal_Dec(node_t *node, type_ptr base_type);

DecList_info_t deal_DecList(node_t *node, type_ptr base_type, int prev_dec_num);

Def_info_t deal_Def(node_t *node);

DefList_info_t deal_DefList(node_t *node, int prev_def_num);

type_ptr deal_Exp(node_t *node);

void deal_all_ExtDef(node_t *node);

void find_undefined_func();

void semantic_analysis(node_t *root);

#endif