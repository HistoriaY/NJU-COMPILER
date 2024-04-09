#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "semantics.h"
#include "typedef.h"

// global basic type ptr
type_ptr type_ptr_int, type_ptr_float;

void init_basic_type_ptr()
{
    type_ptr_int = malloc(sizeof(struct type_s));
    type_ptr_int->kind = type_sys_INT;

    type_ptr_float = malloc(sizeof(struct type_s));
    type_ptr_float->kind = type_sys_FLOAT;
}

// semantic error print
void semantic_error_print(int error_type, int line, char *msg)
{
    printf("Error type %d at Line %d: %s.\n", error_type, line, msg);
}

// anonymous struct name
char *next_anony_struct_name()
{
    static int curr = 0;
    static char name[32];
    sprintf(name, "%d", curr);
    ++curr;
    return name;
}

// semantic analysis

type_ptr deal_StructSpecifier(node_t *node)
{
    // StructSpecifier: STRUCT Tag
    // Tag: ID
    if (node->children[2] == NULL)
    {
        char *tag_name = node->children[1]->children[0]->tev.id;
        symbol_t *s = look_up_symbol(tag_name);
        if (s == NULL)
        {
            semantic_error_print(17, node->first_line, "undefined struct");
            return NULL;
        }
        return s->type;
    }
    // StructSpecifier: STRUCT OptTag LC DefList RC
    // OptTag: ID | empty
    else
    {
        char *tag_name = (node->children[1] == NULL) ? next_anony_struct_name() : node->children[1]->children[0]->tev.id;
        if (look_up_symbol(tag_name))
        {
            semantic_error_print(16, node->first_line, "redefined struct");
            return NULL;
        }
        symbol_t *new_s = malloc(sizeof(symbol_t));
        // symbol name
        new_s->name = strdup(tag_name);
        // new type

        // symbol type

        return new_s->type;
    }
}

type_ptr deal_Specifier(node_t *node)
{
    // Specifier: TYPE | StructSpecifier
    node_t *first_child = node->children[0];
    if (strcmp(first_child->name, "TYPE") == 0)
    {
        if (first_child->tev.type == TYPE_token_INT)
            return type_ptr_int;
        else if (first_child->tev.type == TYPE_token_FLOAT)
            return type_ptr_float;
        return NULL;
    }
    else if (strcmp(first_child->name, "StructSpecifier") == 0)
    {
        return deal_StructSpecifier(first_child);
    }
    return NULL;
}

void deal_ExtDef(node_t *node)
{
    /*
    ExtDef: Specifier ExtDecList SEMI
    | Specifier SEMI
    | Specifier FunDec CompSt
    */
}

VarDec_info_t deal_VarDec(node_t *node, type_ptr base_type)
{
    // VarDec: ID
    node_t *first_child = node->children[0];
    if (strcmp(first_child->name, "ID") == 0)
    {
        VarDec_info_t tmp = {first_child->tev.id, base_type, node};
        return tmp;
    }
    // VarDec: VarDec LB INT RB
    else
    {
        type_ptr tail_type = malloc(sizeof(struct type_s));
        tail_type->kind = type_sys_ARRAY;
        tail_type->u.array.size = node->children[2]->tev.int_val;
        tail_type->u.array.elem_type = base_type;
        VarDec_info_t tmp = deal_VarDec(first_child, tail_type);
        tmp.VarDec_node = node;
        return tmp;
    }
}

Dec_info_t deal_Dec(node_t *node, type_ptr base_type)
{
    // Dec: VarDec | VarDec ASSIGNOP Exp
    Dec_info_t tmp;
    tmp.var_dec_info = deal_VarDec(node->children[0], base_type);
    tmp.assign_Exp = node->children[2];
    tmp.Dec_node = node;
    return tmp;
}

DecList_info_t deal_DecList(node_t *node, type_ptr base_type, int prev_dec_num)
{
    // DecList: Dec | Dec COMMA DecList
    DecList_info_t tmp;
    if (node->children[2] == NULL)
    {
        tmp.dec_info_num = prev_dec_num + 1;
        tmp.dec_infos = malloc(sizeof(Dec_info_t) * tmp.dec_info_num);
        tmp.dec_infos[prev_dec_num] = deal_Dec(node->children[0], base_type);
        tmp.DecList_node = node;
        return tmp;
    }
    else
    {
        tmp = deal_DecList(node->children[2], base_type, prev_dec_num + 1);
        tmp.dec_infos[prev_dec_num] = deal_Dec(node->children[0], base_type);
        tmp.DecList_node = node;
        return tmp;
    }
}

Def_info_t deal_Def(node_t *node)
{
    // Def: Specifier DecList SEMI
    Def_info_t tmp;
    tmp.base_type = deal_Specifier(node->children[0]);
    tmp.dec_list_info = deal_DecList(node->children[1], tmp.base_type, 0);
    tmp.Def_node = node;
    return tmp;
}

DefList_info_t deal_DefList(node_t *node, int prev_def_num)
{
    // DefList: Def DefList | empty
    DefList_info_t tmp;
    if (node->children[1] == NULL)
    {
        tmp.def_info_num = prev_def_num + 1;
        tmp.def_infos = malloc(sizeof(Def_info_t) * tmp.def_info_num);
        tmp.def_infos[prev_def_num] = deal_Def(node->children[0]);
        tmp.DefList_node = node;
        return tmp;
    }
    else
    {
        tmp = deal_DefList(node->children[1], prev_def_num + 1);
        tmp.def_infos[prev_def_num] = deal_Def(node->children[0]);
        tmp.DefList_node = node;
        return tmp;
    }
}

void deal_Exp(node_t *node)
{
    /*
    Exp: Exp ASSIGNOP Exp
    | Exp AND Exp
    | Exp OR Exp
    | Exp RELOP Exp
    | Exp PLUS Exp
    | Exp MINUS Exp
    | Exp STAR Exp
    | Exp DIV Exp
    | LP Exp RP
    | MINUS Exp %prec NEG
    | NOT Exp
    | ID LP Args RP
    | ID LP RP
    | Exp LB Exp RB
    | Exp DOT ID
    | ID
    | INT
    | FLOAT
    */
}

void semantic_dfs(node_t *node)
{
    if (strcmp(node->name, "ExtDef") == 0)
    {
        deal_ExtDef(node);
        return;
    }
    if (strcmp(node->name, "Def") == 0)
    {
        deal_Def(node);
        return;
    }
    if (strcmp(node->name, "Exp") == 0)
    {
        deal_Exp(node);
        return;
    }
    for (int i = 0; i < 7; ++i)
        if (node->children[i] != NULL)
            semantic_dfs(node->children[i]);
}

void semantic_analysis(node_t *root)
{
    init_basic_type_ptr();
    semantic_dfs(root);
}