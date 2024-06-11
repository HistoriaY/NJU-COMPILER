#include "mips.h"
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char *strdup(char *src)
{
    size_t len = strlen(src);
    char *dst = (char *)malloc(len + 1);
    strncpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

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
// $sp bias to $fp in current frame
static int sp_bias;
addr_descriptor_t *push(char *var, int size)
{
    char *code = createFormattedString("addi $sp $sp -%d", size);
    dump_asm_and_free(code);
    sp_bias -= size;
    addr_descriptor_t *ad = malloc(sizeof(addr_descriptor_t));
    ad->reg = reg_null;
    ad->bias = sp_bias;
    insert_ad(strdup(var), ad);
    return ad;
}
void pop(int size)
{
    char *code = createFormattedString("addi $sp $sp %d", size);
    dump_asm_and_free(code);
    sp_bias += size;
}

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

// malloc a reg for var, then load var into that reg
mips_reg_t reg_no(char *var)
{
    static mips_reg_t start = reg_t0;
    static mips_reg_t end = reg_t2;
    static mips_reg_t curr = reg_t0;
    // var maybe #imm
    if (var[0] == '#')
    {
        mips_reg_t ret = curr;
        curr = (curr + 1 - start) % (end - start + 1) + start;
        char *code = createFormattedString("li %s , %s", reg_name[ret], var + 1);
        dump_asm_and_free(code);
        return ret;
    }
    addr_descriptor_t *ad = look_up_ad(var);
    if (ad == NULL)
        ad = push(var, 4);
    ad->reg = curr;
    curr = (curr + 1 - start) % (end - start + 1) + start;
    char *code = createFormattedString("lw %s , %d($fp)", reg_name[ad->reg], ad->bias);
    dump_asm_and_free(code);
    return ad->reg;
}

// get reg name thar var in
#define reg(var) reg_name[reg_no(var)]
// store var in reg back into memory
void store(char *var)
{
    addr_descriptor_t *ad = look_up_ad(var);
    if (ad == NULL)
        exit(2);
    char *code = createFormattedString("sw %s , %d($fp)", reg_name[ad->reg], ad->bias);
    dump_asm_and_free(code);
}

// IR pattern
#define IR_PATTERN_NUM 19
// "[a-zA-Z_]\\w*" simplify to "\\w+" there is no worry to trust in ir.c that no number at the beginning
// !!!maybe exist RE conflict, need to check!!!
char *ir_pattern_str[IR_PATTERN_NUM] = {
    "^LABEL (\\w+) :$",                                 // LABEL x :
    "^FUNCTION (\\w+) :$",                              // FUNCTION f :
    "^(\\w+) := (#?\\w+)$",                             // x := y
    "^(\\w+) := (#?\\w+) \\+ (#?\\w+)$",                // x := y + z
    "^(\\w+) := (#?\\w+) - (#?\\w+)$",                  // x := y - z
    "^(\\w+) := (#?\\w+) \\* (#?\\w+)$",                // x := y * z
    "^(\\w+) := (#?\\w+) / (#?\\w+)$",                  // x := y / z
    "^(\\w+) := &(\\w+)$",                              // x := &y
    "^(\\w+) := \\*(\\w+)$",                            // x := *y
    "^\\*(\\w+) := (#?\\w+)$",                          //*x := y
    "^GOTO (\\w+)$",                                    // GOTO x
    "^IF (\\w+) (>|<|>=|<=|==|!=) (\\w+) GOTO (\\w+)$", // IF x [relop] y GOTO z
    "^RETURN (\\w+)$",                                  // RETURN x
    "^DEC (\\w+) ([0-9]+)$",                            // DEC x [size]
    "^ARG (\\w+)$",                                     // ARG x
    "^(\\w+) := CALL (\\w+)$",                          // x := CALL f
    "^PARAM (\\w+)$",                                   // PARAM x
    "^READ (\\w+)$",                                    // READ x
    "^WRITE (\\w+)$"                                    // WRITE x
};
regex_t ir_pattern[IR_PATTERN_NUM];

void init_ir_pattern()
{
    for (int i = 0; i < IR_PATTERN_NUM; ++i)
    {
        if (regcomp(&ir_pattern[i], ir_pattern_str[i], REG_EXTENDED) != 0)
        {
            printf("init_ir_pattern failed\n");
            exit(2);
        }
    }
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
    char *code = createFormattedString("%s:", f);
    dump_asm_and_free(code);
    free(f);
    next_para_no = 1;
    sp_bias = 0; // reset sp_bias
    dump_asm("addi $sp, $sp, -4");
    dump_asm("sw $fp, 0($sp)");
    dump_asm("move $fp , $sp");
}

// (x) := (y)
void ir2asm_2(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1);
    char *y = sub_str(2);
    if (y[0] == '#')
    {
        char *code = createFormattedString("li %s , %s", reg(x), y + 1);
        dump_asm_and_free(code);
    }
    else
    {
        char *code = createFormattedString("move %s , %s", reg(x), reg(y));
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
        char *code = createFormattedString("addi %s , %s , %s", reg(x), reg(y), z + 1);
        dump_asm_and_free(code);
    }
    else
    {
        char *code = createFormattedString("add %s , %s , %s", reg(x), reg(y), reg(z));
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
        char *code = createFormattedString("addi %s , %s , -%s", reg(x), reg(y), z + 1);
        dump_asm_and_free(code);
    }
    else
    {
        char *code = createFormattedString("sub %s , %s , %s", reg(x), reg(y), reg(z));
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
    char *code = createFormattedString("mul %s , %s , %s", reg(x), reg(y), reg(z));
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
    char *code = createFormattedString("div %s , %s , %s", reg(x), reg(y), reg(z));
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
    char *code = createFormattedString("la %s , %d($fp)", reg(x), look_up_ad(y)->bias);
    dump_asm_and_free(code);
    store(x);
    free(x);
    free(y);
}

// (x) := *(y)
void ir2asm_8(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1), *y = sub_str(2);
    char *code = createFormattedString("lw %s , 0(%s)", reg(x), reg(y));
    dump_asm_and_free(code);
    store(x);
    free(x);
    free(y);
}

//*(x) := (y)
void ir2asm_9(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1), *y = sub_str(2);
    char *code = createFormattedString("sw %s , 0(%s)", reg(y), reg(x));
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
    char *code = createFormattedString("%s %s , %s , %s", asm_op, reg(x), reg(y), z);
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
    char *code1 = createFormattedString("move $v0, %s", reg(x));
    dump_asm_and_free(code1);
    free(x);
    dump_asm("move $sp, $fp");
    dump_asm("lw $fp, 0($sp)");
    dump_asm("addi $sp, $sp, 4");
    dump_asm("jr $ra");
}

// DEC (x) (size)
void ir2asm_13(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1), *s_str = sub_str(2);
    push(x, atoi(s_str));
}

// ARG (x)
void ir2asm_14(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1);
    dump_asm("addi $sp, $sp, -4");
    char *code = createFormattedString("sw %s, 0($sp)", reg(x));
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
    char *code1 = createFormattedString("jal %s", f);
    dump_asm_and_free(code1);
    // restore $ra after jump
    dump_asm("lw $ra , 0($sp)");
    dump_asm("addi $sp, $sp, 4");
    char *code2 = createFormattedString("move %s, $v0", reg(x));
    dump_asm_and_free(code2);
    store(x);
    free(x);
    free(f);
}

// PARAM (x)
void ir2asm_16(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1);
    addr_descriptor_t *ad = push(x, 4);
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
    dump_asm("lw $ra , 0($sp)");
    dump_asm("addi $sp, $sp, 4");
    // save return value
    char *x = sub_str(1);
    char *code2 = createFormattedString("move %s, $v0", reg(x));
    dump_asm_and_free(code2);
    store(x);
    free(x);
}

// WRITE (x)
void ir2asm_18(char *str, size_t nmatch, regmatch_t *pmatch)
{
    char *x = sub_str(1);
    char *code = createFormattedString("move $a0, %s", reg(x));
    dump_asm_and_free(code);
    free(x);
    // save $ra before jump
    dump_asm("addi $sp, $sp, -4");
    dump_asm("sw $ra, 0($sp)");
    // jump
    dump_asm("jal write");
    // restore $ra after jump
    dump_asm("lw $ra , 0($sp)");
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
    do
    {
        int matched = 0;
        for (int i = 0; i < IR_PATTERN_NUM; ++i)
        {
            size_t nmatch = ir_pattern[i].re_nsub + 1;
            regmatch_t *pmatch = malloc(sizeof(regmatch_t) * nmatch);
            if (regexec(&ir_pattern[i], curr->code_str, nmatch, pmatch, 0) == 0)
            {
                // printf("matched! (whole) %.*s", pmatch[0].rm_eo - pmatch[0].rm_so, curr->code_str + pmatch[0].rm_so);
                // for (int i = 1; i < nmatch; ++i)
                //     printf(" (sub%d) %.*s", i, pmatch[i].rm_eo - pmatch[i].rm_so, curr->code_str + pmatch[i].rm_so);
                // printf("\n");
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
