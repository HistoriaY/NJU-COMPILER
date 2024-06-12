#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "ir.h"
#include "typedef.h"
#include "semantics.h"
#include "str_func.h"

// gen single line code
ir_code_t *gen_ir_label_code(const char *const label)
{
    ir_code_t *code = new_empty_code();
    code->code_str = createFormattedString("LABEL %s :", label);
    return code;
}
ir_code_t *gen_ir_jmp_code(const char *const label)
{
    ir_code_t *code = new_empty_code();
    code->code_str = createFormattedString("GOTO %s", label);
    return code;
}

// ir code entrance
ir_code_t *ir_start = NULL;

ir_code_t *new_empty_code()
{
    ir_code_t *temp = malloc(sizeof(ir_code_t));
    temp->code_str = NULL;
    temp->next = temp;
    temp->prev = temp;
    return temp;
}

ir_code_t *merge_code(int count, ...)
{
    va_list args;
    va_start(args, count);
    ir_code_t *head = NULL;
    for (int i = 0; i < count; ++i)
    {
        ir_code_t *code = va_arg(args, ir_code_t *);
        if (code == NULL)
            continue;
        if (head == NULL)
        {
            head = code;
            continue;
        }
        head->prev->next = code;
        code->prev->next = head;
        ir_code_t *temp = code->prev;
        code->prev = head->prev;
        head->prev = temp;
    }
    va_end(args);
    return head;
}

char *new_temp()
{
    static int curr = 1;
    static char buffer[32];
    snprintf(buffer, sizeof(buffer), "t%d", curr);
    ++curr;
    char *temp = malloc(sizeof(char) * (strlen(buffer) + 1));
    strcpy(temp, buffer);
    return temp;
}

char *new_label()
{
    static int curr = 1;
    static char buffer[32];
    snprintf(buffer, sizeof(buffer), "label%d", curr);
    ++curr;
    char *label = malloc(sizeof(char) * (strlen(buffer) + 1));
    strcpy(label, buffer);
    return label;
}

char *get_relop(node_t *node)
{
    static char *g = ">";
    static char *l = "<";
    static char *e = "==";
    static char *ge = ">=";
    static char *le = "<=";
    static char *ne = "!=";
    switch (node->tev.relop_type)
    {
    case relop_g:
        return g;
    case relop_l:
        return l;
    case relop_e:
        return e;
    case relop_ge:
        return ge;
    case relop_le:
        return le;
    case relop_ne:
        return ne;
    default:
        return NULL;
    }
}

char *get_VarDec_name(node_t *node)
{
    // VarDec: ID | VarDec LB INT RB
    if (strcmp(node->children[0]->name, "ID") == 0)
        return node->children[0]->tev.id;
    return get_VarDec_name(node->children[0]);
}

ir_code_t *trans_Cond(node_t *node, char *label_true, char *label_false)
{
    // case Exp of
    // Exp: Exp RELOP Exp
    if (node->children[1] && strcmp(node->children[1]->name, "RELOP") == 0)
    {
        char *t1 = new_temp();
        char *t2 = new_temp();
        ir_code_t *code1 = trans_Exp(node->children[0], t1);
        ir_code_t *code2 = trans_Exp(node->children[2], t2);
        char *op = get_relop(node->children[1]);
        ir_code_t *code3 = new_empty_code();
        code3->code_str = createFormattedString("IF %s %s %s GOTO %s", t1, op, t2, label_true);
        free(t1);
        free(t2);
        return merge_code(4, code1, code2, code3, gen_ir_jmp_code(label_false));
    }
    // Exp: NOT Exp
    if (strcmp(node->children[0]->name, "NOT") == 0)
    {
        return trans_Cond(node->children[1], label_false, label_true);
    }
    // Exp: Exp AND Exp
    if (node->children[1] && strcmp(node->children[1]->name, "AND") == 0)
    {
        char *label1 = new_label();
        ir_code_t *code1 = trans_Cond(node->children[0], label1, label_false);
        ir_code_t *code2 = gen_ir_label_code(label1);
        ir_code_t *code3 = trans_Cond(node->children[2], label_true, label_false);
        free(label1);
        return merge_code(3, code1, code2, code3);
    }
    // Exp: Exp OR Exp
    if (node->children[1] && strcmp(node->children[1]->name, "OR") == 0)
    {
        char *label1 = new_label();
        ir_code_t *code1 = trans_Cond(node->children[0], label_true, label1);
        ir_code_t *code2 = gen_ir_label_code(label1);
        ir_code_t *code3 = trans_Cond(node->children[2], label_true, label_false);
        free(label1);
        return merge_code(3, code1, code2, code3);
    }
    // other cases
    char *t1 = new_temp();
    ir_code_t *code1 = trans_Exp(node, t1);
    ir_code_t *code2 = new_empty_code();
    code2->code_str = createFormattedString("IF %s %s %s GOTO %s", t1, "!=", "#0", label_true);
    free(t1);
    return merge_code(3, code1, code2, gen_ir_jmp_code(label_false));
}

ir_code_t *trans_array_access(node_t *node, char *base, type_ptr *base_type)
{
    // Exp: ID
    if (strcmp(node->children[0]->name, "ID") == 0)
    {
        char *array_name = node->children[0]->tev.id;
        symbol_t *s = look_up_symbol(array_name);
        ir_code_t *code = new_empty_code();
        code->code_str = createFormattedString("%s := %s", base, array_name);
        *base_type = s->type;
        return code;
    }
    // Exp: Exp LB Exp RB
    else if (strcmp(node->children[0]->name, "Exp") == 0)
    {
        // _base _base_type
        char *_base = new_temp();
        type_ptr _base_type;
        ir_code_t *code1 = trans_array_access(node->children[0], _base, &_base_type);
        // bias = index * elem_size
        char *index = new_temp();
        ir_code_t *code2 = trans_Exp(node->children[2], index);
        char *bias = new_temp();
        ir_code_t *code3 = new_empty_code();
        code3->code_str = createFormattedString("%s := %s * #%d", bias, index, _base_type->u.array.elem_type->size);
        free(index);
        // base = _base + bias
        ir_code_t *code4 = new_empty_code();
        code4->code_str = createFormattedString("%s := %s + %s", base, _base, bias);
        free(_base);
        free(bias);
        // base_type
        *base_type = _base_type->u.array.elem_type;
        return merge_code(4, code1, code2, code3, code4);
    }
    else
        exit(1);
}

ir_code_t *trans_array_assign(char *base1, char *base2, type_ptr base_type1, type_ptr base_type2)
{
    // base1 = base2
    // num of assign basic elem
    int num = (base_type1->size <= base_type2->size) ? (base_type1->size >> 2) : (base_type2->size >> 2);
    ir_code_t *code = NULL;
    for (int i = 0; i < num; ++i)
    {
        // addr1
        char *addr1 = new_temp();
        ir_code_t *code1 = new_empty_code();
        code1->code_str = createFormattedString("%s := %s + #%d", addr1, base1, i * 4);
        // addr2
        char *addr2 = new_temp();
        ir_code_t *code2 = new_empty_code();
        code2->code_str = createFormattedString("%s := %s + #%d", addr2, base2, i * 4);
        // *addr1 = *addr2
        char *t = new_temp();
        ir_code_t *code3 = new_empty_code(), *code4 = new_empty_code();
        code3->code_str = createFormattedString("%s := *%s", t, addr2);
        code4->code_str = createFormattedString("*%s := %s", addr1, t);
        free(addr1);
        free(addr2);
        free(t);
        code = merge_code(5, code, code1, code2, code3, code4);
    }
    return code;
}

ir_code_t *trans_Exp(node_t *node, char *place)
{
    // case Exp of
    node_t *first_child = node->children[0];
    node_t *second_child = node->children[1];
    // Exp: ID | INT | FLOAT
    if (strcmp(first_child->name, "ID") == 0 && second_child == NULL)
    {
        ir_code_t *code = new_empty_code();
        code->code_str = createFormattedString("%s := %s", place, node->children[0]->tev.id);
        return code;
    }
    if (strcmp(first_child->name, "INT") == 0)
    {
        ir_code_t *code = new_empty_code();
        code->code_str = createFormattedString("%s := #%d", place, node->children[0]->tev.int_val);
        return code;
    }
    if (strcmp(first_child->name, "FLOAT") == 0)
    {
        ir_code_t *code = new_empty_code();
        code->code_str = createFormattedString("%s := #%f", place, node->children[0]->tev.float_val);
        return code;
    }
    // Exp: Exp1 ASSIGNOP Exp2
    if (strcmp(second_child->name, "ASSIGNOP") == 0)
    {
        type_ptr t = deal_Exp(first_child);
        if (t->kind == type_sys_INT || t->kind == type_sys_FLOAT)
        {
            // Exp1 : ID
            if (strcmp(first_child->children[0]->name, "ID") == 0)
            {
                char *t1 = new_temp();
                ir_code_t *code1 = trans_Exp(node->children[2], t1);
                ir_code_t *code2 = new_empty_code();
                code2->code_str = createFormattedString("%s := %s", first_child->children[0]->tev.id, t1);
                ir_code_t *code3 = new_empty_code();
                code3->code_str = createFormattedString("%s := %s", place, first_child->children[0]->tev.id);
                free(t1);
                return merge_code(3, code1, code2, code3);
            }
            // Exp1 : Exp DOT ID
            else if (strcmp(first_child->children[1]->name, "DOT") == 0)
                exit(1);
            // Exp1 : Exp LB Exp RB
            else if (strcmp(first_child->children[1]->name, "LB") == 0)
            {
                // addr
                char *addr = new_temp();
                type_ptr useless;
                ir_code_t *code1 = trans_array_access(first_child, addr, &useless);
                // *addr = t
                char *t = new_temp();
                ir_code_t *code2 = trans_Exp(node->children[2], t);
                ir_code_t *code3 = new_empty_code();
                code3->code_str = createFormattedString("*%s := %s", addr, t);
                // place = *addr
                ir_code_t *code4 = new_empty_code();
                code4->code_str = createFormattedString("%s := *%s", place, addr);
                free(addr);
                free(t);
                return merge_code(4, code1, code2, code3, code4);
            }
        }
        else if (t->kind == type_sys_ARRAY)
        {
            char *base1 = new_temp(), *base2 = new_temp();
            type_ptr base_type1, base_type2;
            ir_code_t *code1 = trans_array_access(first_child, base1, &base_type1);
            ir_code_t *code2 = trans_array_access(node->children[2], base2, &base_type2);
            ir_code_t *code3 = trans_array_assign(base1, base2, base_type1, base_type2);
            free(base1);
            free(base2);
            return merge_code(3, code1, code2, code3);
        }
        else
            exit(1);
    }
    // Exp: Exp AND Exp | Exp OR Exp | NOT Exp | Exp RELOP Exp
    if (strcmp(second_child->name, "AND") == 0 || strcmp(second_child->name, "OR") == 0 ||
        strcmp(first_child->name, "NOT") == 0 || strcmp(second_child->name, "RELOP") == 0)
    {
        char *label1 = new_label();
        char *label2 = new_label();
        ir_code_t *code0 = new_empty_code();
        code0->code_str = createFormattedString("%s := #0", place);
        ir_code_t *code1 = trans_Cond(node, label1, label2);
        ir_code_t *code2 = gen_ir_label_code(label1);
        ir_code_t *code3 = new_empty_code();
        code3->code_str = createFormattedString("%s := #1", place);
        ir_code_t *code4 = gen_ir_label_code(label2);
        free(label1);
        free(label2);
        return merge_code(5, code0, code1, code2, code3, code4);
    }
    // Exp: Exp PLUS Exp | Exp MINUS Exp | Exp STAR Exp | Exp DIV Exp
    if (strcmp(second_child->name, "PLUS") == 0 || strcmp(second_child->name, "MINUS") == 0 ||
        strcmp(second_child->name, "STAR") == 0 || strcmp(second_child->name, "DIV") == 0)
    {
        char *t1 = new_temp();
        char *t2 = new_temp();
        ir_code_t *code1 = trans_Exp(first_child, t1);
        ir_code_t *code2 = trans_Exp(node->children[2], t2);
        ir_code_t *code3 = new_empty_code();
        if (strcmp(second_child->name, "PLUS") == 0)
            code3->code_str = createFormattedString("%s := %s + %s", place, t1, t2);
        else if (strcmp(second_child->name, "MINUS") == 0)
            code3->code_str = createFormattedString("%s := %s - %s", place, t1, t2);
        else if (strcmp(second_child->name, "STAR") == 0)
            code3->code_str = createFormattedString("%s := %s * %s", place, t1, t2);
        else if (strcmp(second_child->name, "DIV") == 0)
            code3->code_str = createFormattedString("%s := %s / %s", place, t1, t2);
        free(t1);
        free(t2);
        return merge_code(3, code1, code2, code3);
    }
    // Exp: MINUS Exp %prec NEG
    if (strcmp(first_child->name, "MINUS") == 0)
    {
        char *t1 = new_temp();
        ir_code_t *code1 = trans_Exp(second_child, t1);
        ir_code_t *code2 = new_empty_code();
        code2->code_str = createFormattedString("%s := #0 - %s", place, t1);
        free(t1);
        return merge_code(2, code1, code2);
    }
    // Exp: LP Exp RP
    if (strcmp(first_child->name, "LP") == 0)
    {
        return trans_Exp(second_child, place);
    }
    // Exp: ID LP Args RP | ID LP RP
    if (strcmp(second_child->name, "LP") == 0)
    {
        symbol_t *function = look_up_symbol(first_child->tev.id);
        if (node->children[3] == NULL)
        {
            // Exp: ID LP RP
            ir_code_t *code = new_empty_code();
            if (strcmp(function->name, "read") == 0)
                code->code_str = createFormattedString("READ %s", place);
            else
                code->code_str = createFormattedString("%s := CALL %s", place, function->name);
            return code;
        }
        else
        {
            // Exp: ID LP Args RP
            char **arg_list = malloc(sizeof(char *) * function->type->u.function.para_num);
            ir_code_t *code1 = trans_Args(node->children[2], arg_list, 0);
            if (strcmp(function->name, "write") == 0)
            {
                ir_code_t *code2 = new_empty_code();
                code2->code_str = createFormattedString("WRITE %s", arg_list[0]);
                ir_code_t *code3 = new_empty_code();
                code3->code_str = createFormattedString("%s := #0", place);
                // free arg_list and arg_list[i]
                for (int i = 0; i < function->type->u.function.para_num; ++i)
                    free(arg_list[i]);
                free(arg_list);
                return merge_code(3, code1, code2, code3);
            }
            else
            {
                ir_code_t *code2 = NULL;
                for (int i = function->type->u.function.para_num - 1; i >= 0; --i)
                {
                    ir_code_t *code3 = new_empty_code();
                    code3->code_str = createFormattedString("ARG %s", arg_list[i]);
                    code2 = merge_code(2, code2, code3);
                }
                ir_code_t *code3 = new_empty_code();
                code3->code_str = createFormattedString("%s := CALL %s", place, function->name);
                // free arg_list and arg_list[i]
                for (int i = 0; i < function->type->u.function.para_num; ++i)
                    free(arg_list[i]);
                free(arg_list);
                return merge_code(3, code1, code2, code3);
            }
        }
    }
    // Exp: Exp DOT ID
    if (strcmp(second_child->name, "DOT") == 0)
    {
        ir_code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: Exp DOT ID");
        return code;
    }
    // Exp: Exp LB Exp RB
    if (strcmp(second_child->name, "LB") == 0)
    {
        // addr
        char *addr = new_temp();
        type_ptr useless;
        ir_code_t *code1 = trans_array_access(node, addr, &useless);
        // place = *addr
        ir_code_t *code2 = new_empty_code();
        code2->code_str = createFormattedString("%s := *%s", place, addr);
        free(addr);
        return merge_code(2, code1, code2);
    }
    exit(1);
}

ir_code_t *trans_Dec(node_t *node)
{
    // Dec: VarDec | VarDec ASSIGNOP Exp
    char *name = get_VarDec_name(node->children[0]);
    symbol_t *s = look_up_symbol(name);
    ir_code_t *code1 = NULL, *code2 = NULL;
    // DEC size
    if (s->type->kind == type_sys_INT || s->type->kind == type_sys_FLOAT)
        ;
    else if (s->type->kind == type_sys_ARRAY)
    {
        code1 = new_empty_code();
        char *t = new_temp();
        code1->code_str = createFormattedString("DEC %s %d", t, s->type->size);
        ir_code_t *retrieve_addr_code = new_empty_code();
        retrieve_addr_code->code_str = createFormattedString("%s := &%s", s->name, t);
        free(t);
        code1 = merge_code(2, code1, retrieve_addr_code);
    }
    else
        exit(1);
    // assign
    if (node->children[2])
    {
        if (s->type->kind == type_sys_INT || s->type->kind == type_sys_FLOAT)
            code2 = trans_Exp(node->children[2], name);
        else if (s->type->kind == type_sys_ARRAY)
        {
            char *base2 = new_temp();
            type_ptr base_type2;
            code2 = trans_array_access(node->children[2], base2, &base_type2);
            ir_code_t *code3 = trans_array_assign(name, base2, s->type, base_type2);
            free(base2);
            code2 = merge_code(2, code2, code3);
        }
        else
            exit(1);
    }
    return merge_code(2, code1, code2);
}

ir_code_t *trans_DecList(node_t *node)
{
    // DecList: Dec | Dec COMMA DecList
    ir_code_t *code = trans_Dec(node->children[0]);
    if (node->children[2])
        code = merge_code(2, code, trans_DecList(node->children[2]));
    return code;
}

ir_code_t *trans_Def(node_t *node)
{
    // Def: Specifier DecList SEMI
    return trans_DecList(node->children[1]);
}

ir_code_t *trans_DefList(node_t *node)
{
    // DefList: Def DefList | empty
    ir_code_t *code = trans_Def(node->children[0]);
    if (node->children[1] != NULL)
        code = merge_code(2, code, trans_DefList(node->children[1]));
    return code;
}

ir_code_t *trans_StmtList(node_t *node)
{
    // StmtList: Stmt StmtList | empty
    ir_code_t *code = trans_Stmt(node->children[0]);
    if (node->children[1] != NULL)
        code = merge_code(2, code, trans_StmtList(node->children[1]));
    return code;
}

ir_code_t *trans_CompSt(node_t *node)
{
    // CompSt: LC DefList StmtList RC
    ir_code_t *code = NULL;
    if (node->children[1] != NULL)
        code = trans_DefList(node->children[1]);
    if (node->children[2] != NULL)
        code = merge_code(2, code, trans_StmtList(node->children[2]));
    return code;
}

ir_code_t *trans_Stmt(node_t *node)
{
    // case Stmt of
    node_t *first_child = node->children[0];
    // Stmt: Exp SEMI
    if (strcmp(first_child->name, "Exp") == 0)
    {
        // return trans_Exp(first_child, NULL);
        char *t1 = new_temp();
        ir_code_t *code = trans_Exp(first_child, t1);
        free(t1);
        return code;
    }
    // Stmt: CompSt
    if (strcmp(first_child->name, "CompSt") == 0)
    {
        return trans_CompSt(first_child);
    }
    // Stmt: RETURN Exp SEMI
    if (strcmp(first_child->name, "RETURN") == 0)
    {
        char *t1 = new_temp();
        ir_code_t *code1 = trans_Exp(node->children[1], t1);
        ir_code_t *code2 = new_empty_code();
        code2->code_str = createFormattedString("RETURN %s", t1);
        free(t1);
        return merge_code(2, code1, code2);
    }
    // Stmt: WHILE LP Exp RP Stmt
    if (strcmp(first_child->name, "WHILE") == 0)
    {
        char *label1 = new_label(), *label2 = new_label(), *label3 = new_label();
        ir_code_t *code1 = gen_ir_label_code(label1);
        ir_code_t *code2 = trans_Cond(node->children[2], label2, label3);
        ir_code_t *code3 = gen_ir_label_code(label2);
        ir_code_t *code4 = trans_Stmt(node->children[4]);
        ir_code_t *code5 = gen_ir_jmp_code(label1);
        ir_code_t *code6 = gen_ir_label_code(label3);
        free(label1);
        free(label2);
        free(label3);
        return merge_code(6, code1, code2, code3, code4, code5, code6);
    }
    // Stmt: IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
    // Stmt: IF LP Exp RP Stmt ELSE Stmt
    if (strcmp(first_child->name, "IF") == 0)
    {
        if (node->children[5] == NULL)
        {
            char *label1 = new_label(), *label2 = new_label();
            ir_code_t *code1 = trans_Cond(node->children[2], label1, label2);
            ir_code_t *code2 = gen_ir_label_code(label1);
            ir_code_t *code3 = trans_Stmt(node->children[4]);
            ir_code_t *code4 = gen_ir_label_code(label2);
            free(label1);
            free(label2);
            return merge_code(4, code1, code2, code3, code4);
        }
        else
        {
            char *label1 = new_label(), *label2 = new_label(), *label3 = new_label();
            ir_code_t *code1 = trans_Cond(node->children[2], label1, label2);
            ir_code_t *code2 = gen_ir_label_code(label1);
            ir_code_t *code3 = trans_Stmt(node->children[4]);
            ir_code_t *code4 = gen_ir_jmp_code(label3);
            ir_code_t *code5 = gen_ir_label_code(label2);
            ir_code_t *code6 = trans_Stmt(node->children[6]);
            ir_code_t *code7 = gen_ir_label_code(label3);
            free(label1);
            free(label2);
            free(label3);
            return merge_code(7, code1, code2, code3, code4, code5, code6, code7);
        }
    }
    exit(1);
}

ir_code_t *trans_Args(node_t *node, char **arg_list, int pos)
{
    // case Args of
    // Args: Exp | Exp COMMA Args
    type_ptr t = deal_Exp(node->children[0]);
    ir_code_t *code1 = NULL, *code2 = NULL;
    if (t->kind == type_sys_INT || t->kind == type_sys_FLOAT)
    {
        char *t1 = new_temp();
        code1 = trans_Exp(node->children[0], t1);
        arg_list[pos] = t1;
        if (node->children[2])
            code2 = trans_Args(node->children[2], arg_list, pos + 1);
    }
    else if (t->kind == type_sys_ARRAY)
    {
        char *base = new_temp();
        type_ptr useless;
        code1 = trans_array_access(node->children[0], base, &useless);
        arg_list[pos] = base;
        if (node->children[2])
            code2 = trans_Args(node->children[2], arg_list, pos + 1);
    }
    else
        exit(1);
    return merge_code(2, code1, code2);
}

ir_code_t *trans_ParamDec(node_t *node)
{
    // ParamDec: Specifier VarDec
    char *name = get_VarDec_name(node->children[1]);
    ir_code_t *code = new_empty_code();
    code->code_str = createFormattedString("PARAM %s", name);
    return code;
}

ir_code_t *trans_VarList(node_t *node)
{
    // VarList: ParamDec COMMA VarList | ParamDec
    ir_code_t *code1 = trans_ParamDec(node->children[0]);
    if (node->children[2])
        code1 = merge_code(2, code1, trans_VarList(node->children[2]));
    return code1;
}

ir_code_t *trans_FunDec(node_t *node)
{
    // FunDec: ID LP VarList RP | ID LP RP
    ir_code_t *code1 = new_empty_code();
    code1->code_str = createFormattedString("FUNCTION %s :", node->children[0]->tev.id);
    if (strcmp(node->children[2]->name, "VarList") == 0)
        code1 = merge_code(2, code1, trans_VarList(node->children[2]));
    return code1;
}

ir_code_t *trans_ExtDef(node_t *node)
{
    // no need to trans Specifier
    // ExtDef: Specifier SEMI (no this case)
    // ExtDef: Specifier ExtDecList SEMI (no this case)
    if (strcmp(node->children[1]->name, "ExtDecList") == 0)
        exit(1);
    // ExtDef: Specifier FunDec CompSt (only this case) | Specifier FunDec SEMI (no this case)
    else if (strcmp(node->children[1]->name, "FunDec") == 0)
    {
        ir_code_t *code1 = trans_FunDec(node->children[1]);
        ir_code_t *code2 = trans_CompSt(node->children[2]);
        return merge_code(2, code1, code2);
    }
    exit(1);
}

void trans_all_ExtDef(node_t *node)
{
    if (strcmp(node->name, "ExtDef") == 0)
    {
        ir_code_t *code = trans_ExtDef(node);
        if (ir_start == NULL)
            ir_start = code;
        else
            merge_code(2, ir_start, code);
        return;
    }
    for (int i = 0; i < 7; ++i)
        if (node->children[i] != NULL)
            trans_all_ExtDef(node->children[i]);
}

void trans_Program2ir(node_t *root)
{
    trans_all_ExtDef(root);
}

void output_ir_codes(char *ir_file)
{
    FILE *file = fopen(ir_file, "w");
    if (file == NULL)
    {
        fprintf(stderr, "can't open ir file: %s\n", ir_file);
        exit(1);
    }
    ir_code_t *curr = ir_start;
    if (!curr)
    {
        fclose(file);
        return;
    }
    do
    {
        fprintf(file, "%s\n", curr->code_str);
        curr = curr->next;
    } while (curr != ir_start);
    fclose(file);
}