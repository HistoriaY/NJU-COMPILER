#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "ir.h"
#include "typedef.h"

static const char *IR_LABEL_FORMAT = "LABEL %s :";
static const char *IR_FUNC_FORMAT = "FUNCTION %s :";
static const char *IR_COND_JMP_FORMAT = "IF %s %s %s GOTO %s";
static const char *IR_JMP_FORMAT = "GOTO %s";
static const char *IR_RETURN_FORMAT = "RETURN %s";
static const char *IR_PARAM_FORMAT = "PARAM %s";
static const char *IR_DEC_FORMAT = "DEC %s %d";

// gen single line code
code_t *gen_ir_label_code(const char *const label)
{
    code_t *code = new_empty_code();
    code->code_str = createFormattedString(IR_LABEL_FORMAT, label);
    return code;
}
code_t *gen_ir_jmp_code(const char *const label)
{
    code_t *code = new_empty_code();
    code->code_str = createFormattedString(IR_JMP_FORMAT, label);
    return code;
}

// ir code entrance
code_t *ir_start = NULL;

// so magic, so amazing, just use it
char *createFormattedString(const char *format, ...)
{
    // 获取格式化字符串长度
    va_list args;                                  // 创建 va_list 变量，用于存储可变参数列表
    va_start(args, format);                        // 初始化 va_list 变量，与最后一个固定参数之前的可变参数关联起来
    int length = vsnprintf(NULL, 0, format, args); // 获取格式化字符串在不分配内存的情况下的长度
    va_end(args);                                  // 清理 va_list 变量
    // 动态分配内存
    char *str = malloc((length + 1) * sizeof(char)); // 根据格式化字符串长度动态分配内存
    if (str == NULL)
    {
        // 内存分配失败处理
        fprintf(stderr, "Failed to allocate memory\n");
        return NULL;
    }
    // 格式化字符串填充到动态分配的内存中
    va_start(args, format);                   // 初始化 va_list 变量，与最后一个固定参数之前的可变参数关联起来
    vsnprintf(str, length + 1, format, args); // 将格式化字符串填充到动态分配的内存中
    va_end(args);                             // 清理 va_list 变量
    return str;                               // 返回动态分配的字符串
}

code_t *new_empty_code()
{
    code_t *temp = malloc(sizeof(code_t));
    temp->code_str = NULL;
    temp->next = temp;
    temp->prev = temp;
    return temp;
}

code_t *merge_code(int count, ...)
{
    va_list args;
    va_start(args, count);
    code_t *head = NULL;
    for (int i = 0; i < count; ++i)
    {
        code_t *code = va_arg(args, code_t *);
        if (code == NULL)
            continue;
        if (head == NULL)
        {
            head = code;
            continue;
        }
        head->prev->next = code;
        code->prev->next = head;
        code_t *temp = code->prev;
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

code_t *trans_Cond(node_t *node, char *label_true, char *label_false)
{
    // case Exp of
    // Exp: Exp RELOP Exp
    if (node->children[1] && strcmp(node->children[1]->name, "RELOP") == 0)
    {
        char *t1 = new_temp();
        char *t2 = new_temp();
        code_t *code1 = trans_Exp(node->children[0], t1);
        code_t *code2 = trans_Exp(node->children[2], t2);
        char *op = get_relop(node->children[1]);
        code_t *code3 = new_empty_code();
        code3->code_str = createFormattedString(IR_COND_JMP_FORMAT, t1, op, t2, label_true);
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
        code_t *code1 = trans_Cond(node->children[0], label1, label_false);
        code_t *code2 = trans_Cond(node->children[2], label_true, label_false);
        // free(label1);
        return merge_code(3, code1, gen_ir_label_code(label1), code2);
    }
    // Exp: Exp OR Exp
    if (node->children[1] && strcmp(node->children[1]->name, "OR") == 0)
    {
        char *label1 = new_label();
        code_t *code1 = trans_Cond(node->children[0], label_true, label1);
        code_t *code2 = trans_Cond(node->children[2], label_true, label_false);
        // free(label1);
        return merge_code(3, code1, gen_ir_label_code(label1), code2);
    }
    // other cases
    char *t1 = new_temp();
    code_t *code1 = trans_Exp(node, t1);
    code_t *code2 = new_empty_code();
    code2->code_str = createFormattedString(IR_COND_JMP_FORMAT, t1, "!=", "#0", label_true);
    free(t1);
    return merge_code(3, code1, code2, gen_ir_jmp_code(label_false));
}

type_ptr deal_Exp(node_t *node);
code_t *trans_Exp(node_t *node, char *place);

code_t *trans_array_base_addr(node_t *node, char *base, type_ptr *array_type)
{
    // Exp: ID
    if (strcmp(node->children[0]->name, "ID") == 0)
    {
        char *array_name = node->children[0]->tev.id;
        symbol_t *s = look_up_symbol(array_name);
        *array_type = s->type;
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("%s := %s", base, array_name);
        return code;
    }
    // Exp: Exp LB Exp RB
    else if (strcmp(node->children[0]->name, "Exp") == 0)
    {
        // _base _array_type
        char *_base = new_temp();
        type_ptr _array_type;
        code_t *code1 = trans_array_base_addr(node->children[0], _base, &_array_type);
        // bias = index * elem_size
        char *index = new_temp();
        code_t *code2 = trans_Exp(node->children[2], index);
        char *bias = new_temp();
        code_t *code3 = new_empty_code();
        code3->code_str = createFormattedString("%s := %s * #%d", bias, index, _array_type->u.array.elem_type->size);
        free(index);
        // base = _base + bias
        code_t *code4 = new_empty_code();
        code4->code_str = createFormattedString("%s := %s + %s", base, _base, bias);
        free(_base);
        free(bias);
        // array_type
        *array_type = _array_type->u.array.elem_type;
        return merge_code(4, code1, code2, code3, code4);
    }
    else
        exit(1);
}

code_t *trans_Exp(node_t *node, char *place)
{
    // case Exp of
    node_t *first_child = node->children[0];
    node_t *second_child = node->children[1];
    // Exp: ID | INT | FLOAT
    if (strcmp(first_child->name, "ID") == 0 && second_child == NULL)
    {
        symbol_t *s = look_up_symbol(first_child->tev.id);
        type_ptr type = s->type;
        code_t *code = new_empty_code();
        if (type->kind == type_sys_INT || type->kind == type_sys_FLOAT)
            code->code_str = createFormattedString("%s := %s", place, node->children[0]->tev.id);
        else if (type->kind == type_sys_ARRAY)
            code->code_str = createFormattedString("%s := *%s", place, node->children[0]->tev.id);
        else if (type->kind == type_sys_STRUCTURE)
            code->code_str = createFormattedString("%s := %s", place, node->children[0]->tev.id);
        return code;
    }
    if (strcmp(first_child->name, "INT") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("%s := #%d", place, node->children[0]->tev.int_val);
        return code;
    }
    if (strcmp(first_child->name, "FLOAT") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("%s := #%f", place, node->children[0]->tev.float_val);
        return code;
    }
    // Exp: Exp1 ASSIGNOP Exp2
    if (strcmp(second_child->name, "ASSIGNOP") == 0)
    {
        // Exp1 : ID
        if (strcmp(first_child->children[0]->name, "ID") == 0)
        {
            // type_ptr type = deal_Exp(node->children[0]);
            char *t1 = new_temp();
            code_t *code1 = trans_Exp(node->children[2], t1);
            code_t *code2 = new_empty_code();
            code2->code_str = createFormattedString("%s := %s\n%s := %s", first_child->children[0]->tev.id, t1, place, first_child->children[0]->tev.id);
            return merge_code(2, code1, code2);
        }
        // Exp1 : Exp DOT ID
        else if (strcmp(first_child->children[1]->name, "DOT") == 0)
            exit(1);
        // Exp1 : Exp LB Exp RB
        else if (strcmp(first_child->children[1]->name, "LB") == 0)
        {
            // base
            char *base = new_temp();
            type_ptr array_type;
            code_t *code1 = trans_array_base_addr(first_child->children[0], base, &array_type);
            // bias = index * elem_size
            char *index = new_temp();
            code_t *code2 = trans_Exp(first_child->children[2], index);
            char *bias = new_temp();
            code_t *code3 = new_empty_code();
            code3->code_str = createFormattedString("%s := %s * #%d", bias, index, array_type->u.array.elem_type->size);
            // addr = base + bias
            char *addr = new_temp();
            code_t *code4 = new_empty_code();
            code4->code_str = createFormattedString("%s := %s + %s", addr, base, bias);
            // *addr = t
            char *t = new_temp();
            code_t *code5 = trans_Exp(node->children[2], t);
            code_t *code6 = new_empty_code();
            code6->code_str = createFormattedString("*%s := %s", addr, t);
            // place = *addr
            code_t *code7 = new_empty_code();
            code7->code_str = createFormattedString("%s := *%s", place, addr);
            return merge_code(7, code1, code2, code3, code4, code5, code6, code7);
        }
    }
    // Exp: Exp AND Exp | Exp OR Exp | NOT Exp | Exp RELOP Exp
    if (strcmp(second_child->name, "AND") == 0 || strcmp(second_child->name, "OR") == 0 ||
        strcmp(first_child->name, "NOT") == 0 || strcmp(second_child->name, "RELOP") == 0)
    {
        char *label1 = new_label();
        char *label2 = new_label();
        code_t *code0 = new_empty_code();
        code0->code_str = createFormattedString("%s := #0", place);
        code_t *code1 = trans_Cond(node, label1, label2);
        code_t *code2 = gen_ir_label_code(label1);
        code_t *code3 = new_empty_code();
        code3->code_str = createFormattedString("%s := #1", place);
        // free(label1);
        // free(label2);
        return merge_code(5, code0, code1, code2, code3, gen_ir_label_code(label2));
    }
    // Exp: Exp PLUS Exp | Exp MINUS Exp | Exp STAR Exp | Exp DIV Exp
    if (strcmp(second_child->name, "PLUS") == 0 || strcmp(second_child->name, "MINUS") == 0 ||
        strcmp(second_child->name, "STAR") == 0 || strcmp(second_child->name, "DIV") == 0)
    {
        char *t1 = new_temp();
        char *t2 = new_temp();
        code_t *code1 = trans_Exp(first_child, t1);
        code_t *code2 = trans_Exp(node->children[2], t2);
        code_t *code3 = new_empty_code();
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
        code_t *code1 = trans_Exp(second_child, t1);
        code_t *code2 = new_empty_code();
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
            code_t *code = new_empty_code();
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
            // TODO: free arg_list and arg_list[i]
            code_t *code1 = trans_Args(node->children[2], arg_list, 0);
            if (strcmp(function->name, "write") == 0)
            {
                code_t *code2 = new_empty_code();
                code2->code_str = createFormattedString("WRITE %s", arg_list[0]);
                code_t *code3 = new_empty_code();
                code3->code_str = createFormattedString("%s := #0", place);
                return merge_code(3, code1, code2, code3);
            }
            else
            {
                code_t *code2 = new_empty_code();
                code2->code_str = createFormattedString("ARG %s", arg_list[0]);
                for (int i = 1; i < function->type->u.function.para_num; ++i)
                {
                    code_t *code3 = new_empty_code();
                    code3->code_str = createFormattedString("ARG %s", arg_list[i]);
                    code2 = merge_code(2, code2, code3);
                }
                code_t *code3 = new_empty_code();
                code3->code_str = createFormattedString("%s := CALL %s", place, function->name);
                return merge_code(3, code1, code2, code3);
            }
        }
    }
    // Exp: Exp DOT ID
    if (strcmp(second_child->name, "DOT") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: Exp DOT ID");
        return code;
    }
    // Exp: Exp LB Exp RB
    if (strcmp(second_child->name, "LB") == 0)
    {
        // base
        char *base = new_temp();
        type_ptr array_type;
        code_t *code1 = trans_array_base_addr(node->children[0], base, &array_type);
        // bias = index * elem_size
        char *index = new_temp();
        code_t *code2 = trans_Exp(node->children[2], index);
        char *bias = new_temp();
        code_t *code3 = new_empty_code();
        code3->code_str = createFormattedString("%s := %s * #%d", bias, index, array_type->u.array.elem_type->size);
        free(index);
        // addr = base + bias
        char *addr = new_temp();
        code_t *code4 = new_empty_code();
        code4->code_str = createFormattedString("%s := %s + %s", addr, base, bias);
        free(base);
        free(bias);
        // place = *addr
        code_t *code5 = new_empty_code();
        code5->code_str = createFormattedString("%s := *%s", place, addr);
        free(addr);
        return merge_code(5, code1, code2, code3, code4, code5);
    }
}

code_t *trans_Dec(node_t *node)
{
    // Dec: VarDec | VarDec ASSIGNOP Exp
    char *name = get_VarDec_name(node->children[0]);
    symbol_t *s = look_up_symbol(name);
    code_t *code1 = NULL, *code2 = NULL;
    // DEC size
    if (s->type->kind == type_sys_INT || s->type->kind == type_sys_FLOAT)
        ;
    else if (s->type->kind == type_sys_ARRAY)
    {
        code1 = new_empty_code();
        char *t = new_temp();
        code1->code_str = createFormattedString(IR_DEC_FORMAT, t, s->type->size);
        code_t *retrieve_addr_code = new_empty_code();
        retrieve_addr_code->code_str = createFormattedString("%s := &%s", s->name, t);
        free(t);
        code1 = merge_code(2, code1, retrieve_addr_code);
    }
    else
        ;
    // assign
    if (node->children[2])
    {
        if (s->type->kind == type_sys_INT || s->type->kind == type_sys_FLOAT)
            code2 = trans_Exp(node->children[2], name);
        else if (s->type->kind == type_sys_ARRAY)
            exit(1);
        else
            exit(1);
    }
    return merge_code(2, code1, code2);
}

code_t *trans_DecList(node_t *node)
{
    // DecList: Dec | Dec COMMA DecList
    code_t *code = trans_Dec(node->children[0]);
    if (node->children[2])
        code = merge_code(2, code, trans_DecList(node->children[2]));
    return code;
}

code_t *trans_Def(node_t *node)
{
    // Def: Specifier DecList SEMI
    return trans_DecList(node->children[1]);
}

code_t *trans_DefList(node_t *node)
{
    // DefList: Def DefList | empty
    code_t *code = trans_Def(node->children[0]);
    if (node->children[1] != NULL)
        code = merge_code(2, code, trans_DefList(node->children[1]));
    return code;
}

code_t *trans_StmtList(node_t *node)
{
    // StmtList: Stmt StmtList | empty
    code_t *code = trans_Stmt(node->children[0]);
    if (node->children[1] != NULL)
        code = merge_code(2, code, trans_StmtList(node->children[1]));
    return code;
}

code_t *trans_CompSt(node_t *node)
{
    // CompSt: LC DefList StmtList RC
    code_t *code = NULL;
    if (node->children[1] != NULL)
        code = trans_DefList(node->children[1]);
    if (node->children[2] != NULL)
        code = merge_code(2, code, trans_StmtList(node->children[2]));
    return code;
}

code_t *trans_Stmt(node_t *node)
{
    // case Stmt of
    node_t *first_child = node->children[0];
    // Stmt: Exp SEMI
    if (strcmp(first_child->name, "Exp") == 0)
    {
        // return trans_Exp(first_child, NULL);
        char *t1 = new_temp();
        code_t *code = trans_Exp(first_child, t1);
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
        code_t *code1 = trans_Exp(node->children[1], t1);
        code_t *code2 = new_empty_code();
        code2->code_str = createFormattedString(IR_RETURN_FORMAT, t1);
        free(t1);
        return merge_code(2, code1, code2);
    }
    // Stmt: WHILE LP Exp RP Stmt
    if (strcmp(first_child->name, "WHILE") == 0)
    {
        char *label1 = new_label(), *label2 = new_label(), *label3 = new_label();
        code_t *code1 = trans_Cond(node->children[2], label2, label3);
        code_t *code2 = trans_Stmt(node->children[4]);
        // free(label1);
        // free(label2);
        // free(label3);
        return merge_code(6, gen_ir_label_code(label1), code1, gen_ir_label_code(label2), code2, gen_ir_jmp_code(label1), gen_ir_label_code(label3));
    }
    // Stmt: IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
    // Stmt: IF LP Exp RP Stmt ELSE Stmt
    if (strcmp(first_child->name, "IF") == 0)
    {
        if (node->children[5] == NULL)
        {
            char *label1 = new_label(), *label2 = new_label();
            code_t *code1 = trans_Cond(node->children[2], label1, label2);
            code_t *code2 = trans_Stmt(node->children[4]);
            // free(label1);
            // free(label2);
            return merge_code(4, code1, gen_ir_label_code(label1), code2, gen_ir_label_code(label2));
        }
        else
        {
            char *label1 = new_label(), *label2 = new_label(), *label3 = new_label();
            code_t *code1 = trans_Cond(node->children[2], label1, label2);
            code_t *code2 = trans_Stmt(node->children[4]);
            code_t *code3 = trans_Stmt(node->children[6]);
            // free(label1);
            // free(label2);
            // free(label3);
            return merge_code(7, code1, gen_ir_label_code(label1), code2, gen_ir_jmp_code(label3), gen_ir_label_code(label2), code3, gen_ir_label_code(label3));
        }
    }
}

code_t *trans_Args(node_t *node, char **arg_list, int pos)
{
    // case Args of
    // Args: Exp
    if (node->children[2] == NULL)
    {
        char *t1 = new_temp();
        code_t *code1 = trans_Exp(node->children[0], t1);
        arg_list[pos] = t1;
        return code1;
    }
    // Args: Exp COMMA Args
    if (node->children[2])
    {
        char *t1 = new_temp();
        code_t *code1 = trans_Exp(node->children[0], t1);
        arg_list[pos] = t1;
        code_t *code2 = trans_Args(node->children[2], arg_list, pos + 1);
        return merge_code(2, code1, code2);
    }
}

code_t *trans_ParamDec(node_t *node)
{
    // ParamDec: Specifier VarDec
    char *name = get_VarDec_name(node->children[1]);
    code_t *code = new_empty_code();
    code->code_str = createFormattedString(IR_PARAM_FORMAT, name);
    return code;
}

code_t *trans_VarList(node_t *node)
{
    // VarList: ParamDec COMMA VarList | ParamDec
    code_t *code1 = trans_ParamDec(node->children[0]);
    if (node->children[2])
        code1 = merge_code(2, code1, trans_VarList(node->children[2]));
    return code1;
}

code_t *trans_FunDec(node_t *node)
{
    // FunDec: ID LP VarList RP | ID LP RP
    code_t *code1 = new_empty_code();
    code1->code_str = createFormattedString(IR_FUNC_FORMAT, node->children[0]->tev.id);
    if (strcmp(node->children[2]->name, "VarList") == 0)
        code1 = merge_code(2, code1, trans_VarList(node->children[2]));
    return code1;
}

code_t *trans_ExtDef(node_t *node)
{
    // no need to trans Specifier
    // ExtDef: Specifier SEMI (no this case)
    // ExtDef: Specifier ExtDecList SEMI (no this case)
    if (strcmp(node->children[1]->name, "ExtDecList") == 0)
    {
        exit(1);
    }
    // ExtDef: Specifier FunDec CompSt (only this case) | Specifier FunDec SEMI (no this case)
    else if (strcmp(node->children[1]->name, "FunDec") == 0)
    {
        code_t *code1 = trans_FunDec(node->children[1]);
        code_t *code2 = trans_CompSt(node->children[2]);
        return merge_code(2, code1, code2);
    }
}

void trans_all_ExtDef(node_t *node)
{
    if (strcmp(node->name, "ExtDef") == 0)
    {
        code_t *code = trans_ExtDef(node);
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

void trans_Program(node_t *root)
{
    trans_all_ExtDef(root);
}