#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "ir.h"
#include "typedef.h"

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
        code3->code_str = createFormattedString("IF %s %s %s GOTO %s", t1, op, t2, label_true);
        code_t *code4 = new_empty_code();
        code4->code_str = createFormattedString("GOTO %s", label_false);
        free(t1);
        free(t2);
        return merge_code(4, code1, code2, code3, code4);
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
        code_t *code3 = new_empty_code();
        code3->code_str = createFormattedString("LABEL %s", label1);
        free(label1);
        return merge_code(3, code1, code3, code2);
    }
    // Exp: Exp OR Exp
    if (node->children[1] && strcmp(node->children[1]->name, "OR") == 0)
    {
        char *label1 = new_label();
        code_t *code1 = trans_Cond(node->children[0], label_true, label1);
        code_t *code2 = trans_Cond(node->children[2], label_true, label_false);
        code_t *code3 = new_empty_code();
        code3->code_str = createFormattedString("LABEL %s", label1);
        free(label1);
        return merge_code(3, code1, code3, code2);
    }
    // other cases
    char *t1 = new_temp();
    code_t *code1 = trans_Exp(node, t1);
    code_t *code2 = new_empty_code();
    code2->code_str = createFormattedString("IF %s != #0 GOTO %s", t1, label_true);
    code_t *code3 = new_empty_code();
    code3->code_str = createFormattedString("GOTO %s", label_false);
    free(t1);
    return merge_code(3, code1, code2, code3);
}

code_t *trans_Exp(node_t *node, char *place)
{
    // case Exp of
    node_t *first_child = node->children[0];
    node_t *second_child = node->children[1];
    // Exp: ID | INT | FLOAT
    if (strcmp(first_child->name, "ID") == 0 && second_child == NULL)
    {
        code_t *code = new_empty_code();
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
    // Exp: Exp ASSIGNOP Exp
    if (strcmp(second_child->name, "ASSIGNOP") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: Exp ASSIGNOP Exp");
        return code;
    }
    // Exp: Exp AND Exp | Exp OR Exp | NOT Exp
    if (strcmp(second_child->name, "AND") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: Exp AND Exp");
        return code;
    }
    if (strcmp(second_child->name, "OR") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: Exp OR Exp");
        return code;
    }
    if (strcmp(first_child->name, "NOT") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: Exp NOT Exp");
        return code;
    }
    // Exp: Exp PLUS Exp | Exp MINUS Exp | Exp STAR Exp | Exp DIV Exp | Exp RELOP Exp |  MINUS Exp %prec NEG
    if (strcmp(second_child->name, "PLUS") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: Exp PLUS Exp");
        return code;
    }
    if (strcmp(second_child->name, "MINUS") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: Exp MINUS Exp");
        return code;
    }
    if (strcmp(second_child->name, "STAR") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: Exp STAR Exp");
        return code;
    }
    if (strcmp(second_child->name, "DIV") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: Exp DIV Exp");
        return code;
    }
    if (strcmp(second_child->name, "RELOP") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: Exp RELOP Exp");
        return code;
    }
    if (strcmp(first_child->name, "MINUS") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: MINUS Exp");
        return code;
    }
    // Exp: LP Exp RP
    if (strcmp(first_child->name, "LP") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: LP Exp RP");
        return code;
    }
    // Exp: ID LP Args RP | ID LP RP
    if (strcmp(second_child->name, "LP") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: ID LP Args RP | ID LP RP");
        return code;
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
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Exp: Exp LB Exp RB");
        return code;
    }
}

code_t *trans_Stmt(node_t *node)
{
    // case Stmt of
    node_t *first_child = node->children[0];
    // Stmt: Exp SEMI
    if (strcmp(first_child->name, "Exp") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Stmt: Exp SEMI");
        return code;
    }
    // Stmt: CompSt
    if (strcmp(first_child->name, "CompSt") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Stmt: CompSt");
        return code;
    }
    // Stmt: RETURN Exp SEMI
    if (strcmp(first_child->name, "RETURN") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Stmt: RETURN Exp SEMI");
        return code;
    }
    // Stmt: WHILE LP Exp RP Stmt
    if (strcmp(first_child->name, "WHILE") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Stmt: WHILE LP Exp RP Stmt");
        return code;
    }
    // Stmt: IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
    // Stmt: IF LP Exp RP Stmt ELSE Stmt
    if (strcmp(first_child->name, "IF") == 0)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Stmt: IF");
        return code;
    }
}

code_t *trans_Args(node_t *node)
{
    // case Args of
    // Args: Exp
    if (node->children[2] == NULL)
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Args: Exp");
        return code;
    }
    // Args: Exp COMMA Args
    if (node->children[2])
    {
        code_t *code = new_empty_code();
        code->code_str = createFormattedString("TODO: Args: Exp COMMA Args");
        return code;
    }
}

void tmp(node_t *node)
{
    if (strcmp(node->name, "Exp") == 0)
    {
        char *t1 = new_temp();
        code_t *code = trans_Exp(node, t1);
        free(t1);
        if (ir_start == NULL)
            ir_start = code;
        else
            merge_code(2, ir_start, code);
        return;
    }
    for (int i = 0; i < 7; ++i)
        if (node->children[i] != NULL)
            tmp(node->children[i]);
}

void trans_Program(node_t *root)
{
    tmp(root);
}