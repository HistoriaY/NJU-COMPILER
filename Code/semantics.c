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
            semantic_error_print(16, node->first_line, "struct name redefined");
            return NULL;
        }
        symbol_t *new_s = malloc(sizeof(symbol_t));
        // symbol name
        new_s->name = strdup(tag_name);
        // new type
        type_ptr new_type = malloc(sizeof(struct type_s));
        // kind
        new_type->kind = type_sys_STRUCTURE;
        // field
        struct_field_ptr first_field = NULL, prev_field = NULL, curr_field = NULL;
        if (node->children[3] != NULL)
        {
            // DefList -> Def -> DecList -> Dec -> VarDec
            // DefList
            DefList_info_t def_list_info = deal_DefList(node->children[3], 0);
            for (int def_idx = 0; def_idx < def_list_info.def_info_num; ++def_idx)
            {
                // Def
                Def_info_t *def_info = &def_list_info.def_infos[def_idx];
                // DecList
                DecList_info_t *dec_list_info = &def_info->dec_list_info;
                for (int dec_idx = 0; dec_idx < dec_list_info->dec_info_num; ++dec_idx)
                {
                    // Dec
                    Dec_info_t *dec_info = &dec_list_info->dec_infos[dec_idx];
                    // VarDec
                    VarDec_info_t *var_dec_info = &dec_info->var_dec_info;
                    if (look_up_symbol(var_dec_info->id))
                    {
                        semantic_error_print(15, var_dec_info->VarDec_node->first_line, "struct field name redefined");
                        return NULL;
                    }
                    else if (dec_info->assign_Exp)
                    {
                        semantic_error_print(15, var_dec_info->VarDec_node->first_line, "struct field shouldn't have init val");
                        return NULL;
                    }
                    curr_field = malloc(sizeof(struct struct_field_s));
                    curr_field->name = var_dec_info->id;
                    curr_field->type = var_dec_info->type;
                    curr_field->next_field = NULL;
                    // register new field symbol
                    symbol_t *new_fs = malloc(sizeof(symbol_t));
                    new_fs->name = curr_field->name;
                    new_fs->type = curr_field->type;
                    insert_symbol(new_fs);
                    if (def_idx == 0 && dec_idx == 0)
                        first_field = curr_field;
                    if (prev_field)
                        prev_field->next_field = curr_field;
                    prev_field = curr_field;
                }
            }
            // TODO: free heap space malloced in DefList_info_t
        }
        new_type->u.structure.struct_name = new_s->name;
        new_type->u.structure.struct_field = first_field;
        // symbol type
        new_s->type = new_type;
        // register new struct symbol
        insert_symbol(new_s);
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

void deal_ExtDecList(node_t *node, type_ptr base_type)
{
    // ExtDecList: VarDec | VarDec COMMA ExtDecList
    VarDec_info_t var_dec_info = deal_VarDec(node->children[0], base_type);
    if (look_up_symbol(var_dec_info.id))
    {
        semantic_error_print(3, var_dec_info.VarDec_node->first_line, "var name redefined");
        return;
    }
    // register new global symbol
    symbol_t *new_s = malloc(sizeof(symbol_t));
    new_s->name = var_dec_info.id;
    new_s->type = var_dec_info.type;
    insert_symbol(new_s);
    // recursive deal
    if (node->children[2] != NULL)
        deal_ExtDecList(node->children[2], base_type);
}

VarList_info_t deal_VarList(node_t *node, type_ptr return_type, int prev_para_num)
{
    // VarList: ParamDec COMMA VarList | ParamDec
    // ParamDec: Specifier VarDec
    if (node->children[2] == NULL)
    {
        VarList_info_t tmp;
        tmp.para_num = prev_para_num + 1;
        tmp.para_types = malloc(sizeof(type_ptr) * tmp.para_num);
        tmp.VarList_node = node;
        type_ptr base_type = deal_Specifier(node->children[0]->children[0]);
        VarDec_info_t vdi = deal_VarDec(node->children[0]->children[1], base_type);
        if (look_up_symbol(vdi.id))
        {
            semantic_error_print(3, vdi.VarDec_node->first_line, "func para name redefined");
            tmp.para_types[prev_para_num] = NULL;
            return tmp;
        }
        symbol_t *new_s = malloc(sizeof(symbol_t));
        new_s->name = vdi.id;
        new_s->type = vdi.type;
        insert_symbol(new_s);
        tmp.para_types[prev_para_num] = new_s->type;
        return tmp;
    }
    else
    {
        VarList_info_t tmp = deal_VarList(node->children[2], return_type, prev_para_num + 1);
        tmp.VarList_node = node;
        type_ptr base_type = deal_Specifier(node->children[0]->children[0]);
        VarDec_info_t vdi = deal_VarDec(node->children[0]->children[1], base_type);
        if (look_up_symbol(vdi.id))
        {
            semantic_error_print(3, vdi.VarDec_node->first_line, "func para name redefined");
            tmp.para_types[prev_para_num] = NULL;
            return tmp;
        }
        symbol_t *new_s = malloc(sizeof(symbol_t));
        new_s->name = vdi.id;
        new_s->type = vdi.type;
        insert_symbol(new_s);
        tmp.para_types[prev_para_num] = new_s->type;
        return tmp;
    }
}

void deal_FunDec(node_t *node, type_ptr return_type, int is_definition)
{
    char *func_name = node->children[0]->tev.id;
    symbol_t *s = look_up_symbol(func_name);
    if (s && s->type->kind == type_sys_FUNCTION)
    {
        semantic_error_print(4, node->children[0]->first_line, "func name redefined");
        return;
    }
    // if (!is_definition)
    // {
    //     // check declare consistency
    // }
    // else
    // {
    //     // check re-definition

    //     // check declare-definition consistency
    // }
    // FunDec: ID LP VarList RP
    if (node->children[3] != NULL)
    {
        VarList_info_t vli = deal_VarList(node->children[2], return_type, 0);
        type_ptr new_type = malloc(sizeof(struct type_s));
        new_type->kind = type_sys_FUNCTION;
        new_type->u.function.return_type = return_type;
        new_type->u.function.para_num = vli.para_num;
        new_type->u.function.para_types = vli.para_types;
        symbol_t *new_s = malloc(sizeof(symbol_t));
        new_s->name = func_name;
        new_s->type = new_type;
        insert_symbol(new_s);
    }
    // FunDec: ID LP RP
    else
    {
        type_ptr new_type = malloc(sizeof(struct type_s));
        new_type->kind = type_sys_FUNCTION;
        new_type->u.function.return_type = return_type;
        new_type->u.function.para_num = 0;
        new_type->u.function.para_types = NULL;
        symbol_t *new_s = malloc(sizeof(symbol_t));
        new_s->name = func_name;
        new_s->type = new_type;
        insert_symbol(new_s);
    }
}

void deal_ExtDef(node_t *node)
{
    // ExtDef: Specifier SEMI
    type_ptr base_type = deal_Specifier(node->children[0]);
    // ExtDef: Specifier ExtDecList SEMI
    if (strcmp(node->children[1]->name, "ExtDecList") == 0)
    {
        deal_ExtDecList(node->children[1], base_type);
    }
    // ExtDef: Specifier FunDec CompSt | Specifier FunDec
    else if (strcmp(node->children[1]->name, "FunDec") == 0)
    {
        int is_definition = (node->children[2] == NULL) ? 0 : 1;
        deal_FunDec(node->children[1], base_type, is_definition);
        if (node->children[2])
            semantic_dfs(node->children[2]);
    }
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
    // Exp: Exp ASSIGNOP Exp

    // Exp: Exp AND Exp | Exp OR Exp| NOT Exp

    // Exp: Exp PLUS Exp | Exp MINUS Exp | Exp STAR Exp | Exp DIV Exp| Exp RELOP Exp| LP Exp RP| MINUS Exp %prec NEG

    // Exp: ID | INT | FLOAT

    // Exp: ID LP Args RP | ID LP RP

    // Exp: Exp DOT ID

    // Exp: Exp LB Exp RB
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
        // Def -> DecList -> Dec -> VarDec
        // Def
        Def_info_t def_info = deal_Def(node);
        // DecList
        DecList_info_t *dec_list_info = &def_info.dec_list_info;
        for (int dec_idx = 0; dec_idx < dec_list_info->dec_info_num; ++dec_idx)
        {
            // Dec
            Dec_info_t *dec_info = &dec_list_info->dec_infos[dec_idx];
            // VarDec
            VarDec_info_t *var_dec_info = &dec_info->var_dec_info;
            if (look_up_symbol(var_dec_info->id))
            {
                semantic_error_print(3, var_dec_info->VarDec_node->first_line, "var name redefined");
                return;
            }
            // register new local symbol
            symbol_t *new_s = malloc(sizeof(symbol_t));
            new_s->name = var_dec_info->id;
            new_s->type = var_dec_info->type;
            insert_symbol(new_s);
        }
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