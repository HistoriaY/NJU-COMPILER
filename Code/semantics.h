#include "typedef.h"

typedef struct VarDec_info_s
{
    // VarDec: ID
    // VarDec: VarDec LB INT RB
    char *id;
    type_ptr type;
    node_t *VarDec_node;
} VarDec_info_t;

typedef struct Dec_info_s
{
    // Dec: VarDec | VarDec ASSIGNOP Exp
    VarDec_info_t var_dec_info;
    node_t *assign_Exp;
    node_t *Dec_node;
} Dec_info_t;

typedef struct DecList_info_s
{
    // DecList: Dec | Dec COMMA DecList
    int dec_info_num;
    Dec_info_t *dec_infos;
    node_t *DecList_node;
} DecList_info_t;

typedef struct Def_info_s
{
    // Def: Specifier DecList SEMI
    type_ptr base_type;
    DecList_info_t dec_list_info;
    node_t *Def_node;
} Def_info_t;

typedef struct DefList_info_s
{
    // DefList: Def DefList | empty
    int def_info_num;
    Def_info_t *def_infos;
    node_t *DefList_node;
} DefList_info_t;

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
    int para_num;
    type_ptr *para_types;
    node_t *VarList_node;
} VarList_info_t;

type_ptr deal_StructSpecifier(node_t *node);

type_ptr deal_Specifier(node_t *node);

void deal_ExtDef(node_t *node);

VarDec_info_t deal_VarDec(node_t *node, type_ptr base_type);

Dec_info_t deal_Dec(node_t *node, type_ptr base_type);

DecList_info_t deal_DecList(node_t *node, type_ptr base_type, int prev_dec_num);

void deal_ExtDecList(node_t *node, type_ptr base_type);

Def_info_t deal_Def(node_t *node);

DefList_info_t deal_DefList(node_t *node, int prev_def_num);

type_ptr deal_Exp(node_t *node);

void semantic_analysis(node_t *root);