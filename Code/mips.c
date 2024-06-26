#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mips.h"
#include "str_func.h"
#include "ir_pattern.h"

static const char start_asm[] =
    ".data\n"
    "_prompt: .asciiz \"Enter an integer:\"\n"
    "_ret: .asciiz \"\\n\"\n"
    ".globl main\n"
    ".text\n"
    "read:\n"
    "li $v0, 4\n"
    "la $a0, _prompt\n"
    "syscall\n"
    "li $v0, 5\n"
    "syscall\n"
    "jr $ra\n"
    "write:\n"
    "li $v0, 1\n"
    "syscall\n"
    "li $v0, 4\n"
    "la $a0, _ret\n"
    "syscall\n"
    "move $v0, $0\n"
    "jr $ra";

// asm file
static FILE *file;
#define dump_asm(str) fprintf(file, "%s\n", str)
#define dump_asm_and_free(str) \
    dump_asm(str);             \
    free(str)

// address descriptor table
HashTable_t *ad_table;
void init_ad_table()
{
    ad_table = hash_table_create(str_hash, str_compare);
}

addr_descriptor_t *look_up_ad(char *var)
{
    return (addr_descriptor_t *)hash_table_search(ad_table, var);
}

void insert_ad(char *var, addr_descriptor_t *ad)
{
    if (look_up_ad(var))
        return;
    hash_table_insert(ad_table, var, ad);
}

// register descriptor table
// reg_descriptor_t reg_desc[REG_NUM];

// register name
char *reg_name[REG_NUM] = {
    "$zero", "$at",
    "$v0", "$v1",
    "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$t8", "$t9",
    "$k0", "$k1",
    "$gp", "$sp", "$fp", "$ra",
    "$reg_null"};

// malloc a reg for var, if load is set, then load var into that reg first
mips_reg_t reg_no(char *var, int load)
{
    static mips_reg_t start = reg_t0;
    static mips_reg_t end = reg_t2;
    static mips_reg_t curr = reg_t0;
    // var maybe #imm
    if (var[0] == '#')
    {
        mips_reg_t ret = curr;
        curr = (curr + 1 - start) % (end - start + 1) + start;
        char *code = createFormattedString("li %s, %s", reg_name[ret], var + 1);
        dump_asm_and_free(code);
        return ret;
    }
    addr_descriptor_t *ad = look_up_ad(var);
    if (ad == NULL)
        exit(2);
    ad->reg = curr;
    curr = (curr + 1 - start) % (end - start + 1) + start;
    if (load)
    {
        char *code = createFormattedString("lw %s, %d($fp)", reg_name[ad->reg], ad->bias);
        dump_asm_and_free(code);
    }
    return ad->reg;
}

// get reg name thar var in, if load is set, then load var into that reg first
#define reg(var, load) reg_name[reg_no(var, load)]
// store var in reg back into memory
void store(char *var)
{
    addr_descriptor_t *ad = look_up_ad(var);
    if (ad == NULL)
        exit(2);
    char *code = createFormattedString("sw %s, %d($fp)", reg_name[ad->reg], ad->bias);
    dump_asm_and_free(code);
}

#define pmatch_len(i) (pmatch[i].rm_eo - pmatch[i].rm_so)
#define pmatch_str(i) (str + pmatch[i].rm_so)
#define sub_str(i) createFormattedString("%.*s", pmatch_len(i), pmatch_str(i))

// LABEL (x) :
void ir2asm_0(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1);
    char *code = createFormattedString("%s:", x);
    dump_asm_and_free(code);
    free(x);
}

static int next_para_no;
// FUNCTION (f) :
void ir2asm_1(char *str, size_t nmatch, regmatch_t *pmatch)
{
    dump_asm("");
    char *f = sub_str(1);
    char *code;
    if (strcmp(f, "main") == 0)
        code = createFormattedString("%s:", f);
    else
        code = createFormattedString("_%s:", f);
    dump_asm_and_free(code);
    dump_asm("addi $sp, $sp, -4");
    dump_asm("sw $fp, 0($sp)");
    dump_asm("move $fp, $sp");
    code = createFormattedString("addi $sp, $sp, %d", look_up_ad(f)->bias);
    dump_asm_and_free(code);
    free(f);
    next_para_no = 1;
}

// (x) := (y)
void ir2asm_2(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1);
    char *y = sub_str(2);
    if (y[0] == '#')
    {
        char *code = createFormattedString("li %s, %s", reg(x, 0), y + 1);
        dump_asm_and_free(code);
    }
    else
    {
        char *code = createFormattedString("move %s, %s", reg(x, 0), reg(y, 1));
        dump_asm_and_free(code);
    }
    store(x);
    free(x);
    free(y);
}

// (x) := (y) + (z)
void ir2asm_3(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1), *y = sub_str(2), *z = sub_str(3);
    if (z[0] == '#')
    {
        char *code = createFormattedString("addi %s, %s, %s", reg(x, 0), reg(y, 1), z + 1);
        dump_asm_and_free(code);
    }
    else
    {
        char *code = createFormattedString("add %s, %s, %s", reg(x, 0), reg(y, 1), reg(z, 1));
        dump_asm_and_free(code);
    }
    store(x);
    free(x);
    free(y);
    free(z);
}

// (x) := (y) - (z)
void ir2asm_4(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1), *y = sub_str(2), *z = sub_str(3);
    if (z[0] == '#')
    {
        char *code = createFormattedString("addi %s, %s, -%s", reg(x, 0), reg(y, 1), z + 1);
        dump_asm_and_free(code);
    }
    else
    {
        char *code = createFormattedString("sub %s, %s, %s", reg(x, 0), reg(y, 1), reg(z, 1));
        dump_asm_and_free(code);
    }
    store(x);
    free(x);
    free(y);
    free(z);
}

// (x) := (y) * (z)
void ir2asm_5(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1), *y = sub_str(2), *z = sub_str(3);
    char *code = createFormattedString("mul %s, %s, %s", reg(x, 0), reg(y, 1), reg(z, 1));
    dump_asm_and_free(code);
    store(x);
    free(x);
    free(y);
    free(z);
}

// (x) := (y) / (z)
void ir2asm_6(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1), *y = sub_str(2), *z = sub_str(3);
    char *code = createFormattedString("div %s, %s, %s", reg(x, 0), reg(y, 1), reg(z, 1));
    dump_asm_and_free(code);
    store(x);
    free(x);
    free(y);
    free(z);
}

// (x) := &(y)
void ir2asm_7(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1), *y = sub_str(2);
    char *code = createFormattedString("la %s, %d($fp)", reg(x, 0), look_up_ad(y)->bias);
    dump_asm_and_free(code);
    store(x);
    free(x);
    free(y);
}

// (x) := *(y)
void ir2asm_8(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1), *y = sub_str(2);
    char *code = createFormattedString("lw %s, 0(%s)", reg(x, 0), reg(y, 1));
    dump_asm_and_free(code);
    store(x);
    free(x);
    free(y);
}

//*(x) := (y)
void ir2asm_9(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1), *y = sub_str(2);
    char *code = createFormattedString("sw %s, 0(%s)", reg(y, 1), reg(x, 1));
    dump_asm_and_free(code);
    free(x);
    free(y);
}

// GOTO (x)
void ir2asm_10(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1);
    char *code = createFormattedString("j %s", x);
    dump_asm_and_free(code);
    free(x);
}

// IF (x) (relop) (y) GOTO (z)
void ir2asm_11(char *str, size_t nmatch, regmatch_t *pmatch)
{
    static char *bgt = "bgt";
    static char *blt = "blt";
    static char *beq = "beq";
    static char *bge = "bge";
    static char *ble = "ble";
    static char *bne = "bne";
    char *x = sub_str(1), *relop = sub_str(2), *y = sub_str(3), *z = sub_str(4);
    char *asm_op;
    if (strcmp(relop, "==") == 0)
        asm_op = beq;
    else if (strcmp(relop, "!=") == 0)
        asm_op = bne;
    else if (strcmp(relop, ">") == 0)
        asm_op = bgt;
    else if (strcmp(relop, "<") == 0)
        asm_op = blt;
    else if (strcmp(relop, ">=") == 0)
        asm_op = bge;
    else if (strcmp(relop, "<=") == 0)
        asm_op = ble;
    else
        asm_op = NULL;
    char *code = createFormattedString("%s %s, %s, %s", asm_op, reg(x, 1), reg(y, 1), z);
    dump_asm_and_free(code);
    free(x);
    free(relop);
    free(y);
    free(z);
}

// RETURN (x)
void ir2asm_12(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1);
    char *code = createFormattedString("move $v0, %s", reg(x, 1));
    dump_asm_and_free(code);
    free(x);
    dump_asm("move $sp, $fp");
    dump_asm("lw $fp, 0($sp)");
    dump_asm("addi $sp, $sp, 4");
    dump_asm("jr $ra");
}

// DEC (x) (size)
void ir2asm_13(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // do nothing
}

static int curr_arg_size = 0;
// ARG (x)
void ir2asm_14(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1);
    dump_asm("addi $sp, $sp, -4");
    curr_arg_size += 4;
    char *code = createFormattedString("sw %s, 0($sp)", reg(x, 1));
    dump_asm_and_free(code);
    free(x);
}

// (x) := CALL (f)
void ir2asm_15(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1), *f = sub_str(2);
    // save $ra before jump
    dump_asm("addi $sp, $sp, -4");
    dump_asm("sw $ra, 0($sp)");
    // jump
    char *code1;
    if (strcmp(f, "main") == 0)
        code1 = createFormattedString("jal %s", f);
    else
        code1 = createFormattedString("jal _%s", f);
    dump_asm_and_free(code1);
    // restore $ra after jump
    dump_asm("lw $ra, 0($sp)");
    dump_asm("addi $sp, $sp, 4");
    // pop args
    char *pop_args = createFormattedString("addi $sp, $sp, %d", curr_arg_size);
    dump_asm_and_free(pop_args);
    curr_arg_size = 0;
    char *code2 = createFormattedString("move %s, $v0", reg(x, 0));
    dump_asm_and_free(code2);
    store(x);
    free(x);
    free(f);
}

// PARAM (x)
void ir2asm_16(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1);
    addr_descriptor_t *ad = look_up_ad(x);
    char *code1 = createFormattedString("lw $t8, %d($fp)", 4 + (next_para_no++) * 4);
    dump_asm_and_free(code1);
    char *code2 = createFormattedString("sw $t8, %d($fp)", ad->bias);
    dump_asm_and_free(code2);
    free(x);
}

// READ (x)
void ir2asm_17(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // save $ra before jump
    dump_asm("addi $sp, $sp, -4");
    dump_asm("sw $ra, 0($sp)");
    // jump
    dump_asm("jal read");
    // restore $ra after jump
    dump_asm("lw $ra, 0($sp)");
    dump_asm("addi $sp, $sp, 4");
    // save return value
    char *x = sub_str(1);
    char *code2 = createFormattedString("move %s, $v0", reg(x, 0));
    dump_asm_and_free(code2);
    store(x);
    free(x);
}

// WRITE (x)
void ir2asm_18(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1);
    char *code = createFormattedString("move $a0, %s", reg(x, 1));
    dump_asm_and_free(code);
    free(x);
    // save $ra before jump
    dump_asm("addi $sp, $sp, -4");
    dump_asm("sw $ra, 0($sp)");
    // jump
    dump_asm("jal write");
    // restore $ra after jump
    dump_asm("lw $ra, 0($sp)");
    dump_asm("addi $sp, $sp, 4");
}

typedef void (*trans_func_ptr)(char *str, size_t nmatch, regmatch_t *pmatch);
trans_func_ptr ir2asm[IR_PATTERN_NUM] = {
    ir2asm_0, ir2asm_1, ir2asm_2, ir2asm_3,
    ir2asm_4, ir2asm_5, ir2asm_6, ir2asm_7,
    ir2asm_8, ir2asm_9, ir2asm_10, ir2asm_11,
    ir2asm_12, ir2asm_13, ir2asm_14, ir2asm_15,
    ir2asm_16, ir2asm_17, ir2asm_18};

void trans_all_ir()
{
    dump_asm(start_asm);
    ir_code_t *curr = ir_start;
    if (!curr)
        return;
    // $sp bias to $fp in current frame
    static int sp_bias;
    // first pass, record var bias to $fp in each function stack
    static char *curr_func = NULL;
    do
    {
        int matched = 0;
        for (int i = 0; i < IR_PATTERN_NUM; ++i)
        {
            size_t nmatch = ir_pattern[i].re_nsub + 1;
            regmatch_t *pmatch = malloc(sizeof(regmatch_t) * nmatch);
            if (regexec(&ir_pattern[i], curr->code_str, nmatch, pmatch, 0) == 0)
            {
                char *str = curr->code_str;
                char *first = sub_str(1);
                // FUNCTION (f) :
                if (i == 1)
                {
                    if (curr_func)
                    {
                        addr_descriptor_t *ad = malloc(sizeof(addr_descriptor_t));
                        ad->bias = sp_bias;
                        ad->reg = reg_null;
                        insert_ad(curr_func, ad);
                    }
                    curr_func = strdup(first);
                    sp_bias = 0; // reset sp_bias
                }
                // := ||  DEC (x) (size) || (x) := CALL (f) || PARAM(x) || READ (x)
                else if ((2 <= i && i <= 9) || i == 13 || i == 15 || i == 16 || i == 17)
                {
                    if (!look_up_ad(first))
                    {
                        if (i == 13)
                        {
                            char *second = sub_str(2);
                            sp_bias -= atoi(second);
                            free(second);
                        }
                        else
                            sp_bias -= 4;
                        addr_descriptor_t *ad = malloc(sizeof(addr_descriptor_t));
                        ad->bias = sp_bias;
                        ad->reg = reg_null;
                        insert_ad(strdup(first), ad);
                    }
                }
                matched = 1;
                free(first);
                free(pmatch);
                break;
            }
            free(pmatch);
        }
        if (!matched)
        {
            printf("unable to recognize ir: %s\n", curr->code_str);
            exit(2);
        }
        curr = curr->next;
    } while (curr != ir_start);
    // last func
    addr_descriptor_t *ad = malloc(sizeof(addr_descriptor_t));
    ad->bias = sp_bias;
    ad->reg = reg_null;
    insert_ad(curr_func, ad);
    // second pass, dump asm
    // curr = ir_start;
    do
    {
        int matched = 0;
        for (int i = 0; i < IR_PATTERN_NUM; ++i)
        {
            size_t nmatch = ir_pattern[i].re_nsub + 1;
            regmatch_t *pmatch = malloc(sizeof(regmatch_t) * nmatch);
            if (regexec(&ir_pattern[i], curr->code_str, nmatch, pmatch, 0) == 0)
            {
                ir2asm[i](curr->code_str, nmatch, pmatch);
                matched = 1;
                free(pmatch);
                break;
            }
            free(pmatch);
        }
        if (!matched)
        {
            printf("unable to recognize ir: %s\n", curr->code_str);
            exit(2);
        }
        curr = curr->next;
    } while (curr != ir_start);
}

void trans_ir2asm(char *asm_file)
{
    init_ir_pattern();
    init_ad_table();
    file = fopen(asm_file, "w");
    if (file == NULL)
    {
        fprintf(stderr, "can't open asm file: %s\n", asm_file);
        exit(2);
    }
    trans_all_ir();
    fclose(file);
}
