#include "mips.h"
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// asm file
static FILE *file;
#define dump_asm(str) fprintf(file, "%s\n", str)
#define dump_asm_and_free(str) \
    dump_asm(str);             \
    free(str)
// $sp bias to $fp
int sp_bias;

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
    {
        ad = malloc(sizeof(addr_descriptor_t));
        ad->reg = reg_null;
        // malloc stack space for var
        sp_bias -= 4;
        ad->bias = sp_bias;
        insert_ad(createFormattedString("%s", var), ad);
    }
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

void ir2asm_0(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // LABEL (x) :
    char *x_str = sub_str(1);
    char *code = createFormattedString("%s:", x_str);
    dump_asm_and_free(code);
    free(x_str);
}

void ir2asm_1(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // FUNCTION (f) :
    sp_bias = -4; // reset sp_bias
    char *f_str = sub_str(1);
    char *code = createFormattedString("%s:", f_str);
    dump_asm_and_free(code);
    free(f_str);
}

void ir2asm_2(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // (x) := (y)
    char *x_str = sub_str(1);
    char *y_str = sub_str(2);
    if (y_str[0] == '#')
    {
        char *code = createFormattedString("li %s , %s", reg(x_str), y_str + 1);
        dump_asm_and_free(code);
    }
    else
    {
        char *code = createFormattedString("move %s , %s", reg(x_str), reg(y_str));
        dump_asm_and_free(code);
    }
    store(x_str);
    free(x_str);
    free(y_str);
}

void ir2asm_3(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // (x) := (y) + (z)
    char *x_str = sub_str(1), *y_str = sub_str(2), *z_str = sub_str(3);
    if (z_str[0] == '#')
    {
        char *code = createFormattedString("addi %s , %s , %s", reg(x_str), reg(y_str), z_str + 1);
        dump_asm_and_free(code);
    }
    else
    {
        char *code = createFormattedString("add %s , %s , %s", reg(x_str), reg(y_str), reg(z_str));
        dump_asm_and_free(code);
    }
    store(x_str);
    free(x_str);
    free(y_str);
    free(z_str);
}

void ir2asm_4(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // (x) := (y) - (z)
    char *x_str = sub_str(1), *y_str = sub_str(2), *z_str = sub_str(3);
    if (z_str[0] == '#')
    {
        char *code = createFormattedString("addi %s , %s , -%s", reg(x_str), reg(y_str), z_str + 1);
        dump_asm_and_free(code);
    }
    else
    {
        char *code = createFormattedString("sub %s , %s , %s", reg(x_str), reg(y_str), reg(z_str));
        dump_asm_and_free(code);
    }
    store(x_str);
    free(x_str);
    free(y_str);
    free(z_str);
}

void ir2asm_5(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // (x) := (y) * (z)
    char *x_str = sub_str(1), *y_str = sub_str(2), *z_str = sub_str(3);
    char *code = createFormattedString("mul %s , %s , %s", reg(x_str), reg(y_str), reg(z_str));
    dump_asm_and_free(code);
    store(x_str);
    free(x_str);
    free(y_str);
    free(z_str);
}

void ir2asm_6(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // (x) := (y) / (z)
    char *x_str = sub_str(1), *y_str = sub_str(2), *z_str = sub_str(3);
    char *code = createFormattedString("div %s , %s , %s", reg(x_str), reg(y_str), reg(z_str));
    dump_asm_and_free(code);
    store(x_str);
    free(x_str);
    free(y_str);
    free(z_str);
}

void ir2asm_7(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // (x) := &(y)
    char *x_str = sub_str(1), *y_str = sub_str(2);
    char *code = createFormattedString("la %s , %d($fp)", reg(x_str), look_up_ad(y_str)->bias);
    dump_asm_and_free(code);
    store(x_str);
    free(x_str);
    free(y_str);
}

void ir2asm_8(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // (x) := *(y)
    char *x_str = sub_str(1), *y_str = sub_str(2);
    char *code = createFormattedString("lw %s , 0(%s)", reg(x_str), reg(y_str));
    dump_asm_and_free(code);
    store(x_str);
    free(x_str);
    free(y_str);
}

void ir2asm_9(char *str, size_t nmatch, regmatch_t *pmatch)
{
    //*(x) := (y)
    char *x_str = sub_str(1), *y_str = sub_str(2);
    char *code = createFormattedString("sw %s , 0(%s)", reg(y_str), reg(x_str));
    dump_asm_and_free(code);
    free(x_str);
    free(y_str);
}

void ir2asm_10(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // GOTO (x)
    char *x_str = sub_str(1);
    char *code = createFormattedString("j %s", x_str);
    dump_asm_and_free(code);
    free(x_str);
}

void ir2asm_11(char *str, size_t nmatch, regmatch_t *pmatch)
{
    static char *bgt = "bgt";
    static char *blt = "blt";
    static char *beq = "beq";
    static char *bge = "bge";
    static char *ble = "ble";
    static char *bne = "bne";
    // IF (x) (relop) (y) GOTO (z)
    char *x_str = sub_str(1), *relop = sub_str(2), *y_str = sub_str(3), *z_str = sub_str(4);
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
    char *code = createFormattedString("%s %s , %s , %s", asm_op, reg(x_str), reg(y_str), z_str);
    dump_asm_and_free(code);
    free(x_str);
    free(relop);
    free(y_str);
    free(z_str);
}

void ir2asm_12(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // RETURN(x)
    char *x_str = sub_str(1);
    char *code1 = createFormattedString("move $v0 , %s", reg(x_str));
    dump_asm_and_free(code1);
    dump_asm("jr $ra");
    free(x_str);
}

void ir2asm_13(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // DEC (x) (size)
    char *x_str = sub_str(1), *s_str = sub_str(2);
    addr_descriptor_t *ad = malloc(sizeof(addr_descriptor_t));
    ad->reg = reg_null;
    sp_bias -= atoi(s_str);
    ad->bias = sp_bias;
    insert_ad(createFormattedString("%s", x_str), ad);
}

void ir2asm_14(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // ARG x
    printf("TODO: ir2asm func %d\n", 14);
}

void ir2asm_15(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // (x) := CALL (f)
    char *x_str = sub_str(1), *f_str = sub_str(2);
    // save $ra before jump
    sp_bias -= 4;
    char *save_ra = createFormattedString("sw $ra , %d($fp)", sp_bias);
    dump_asm_and_free(save_ra);
    // jump
    char *code1 = createFormattedString("jal %s", f_str);
    dump_asm_and_free(code1);
    // restore $ra after jump
    char *restore_ra = createFormattedString("lw $ra , %d($fp)", sp_bias);
    dump_asm_and_free(restore_ra);
    char *code2 = createFormattedString("move %s , $v0", reg(x_str));
    dump_asm_and_free(code2);
    store(x_str);
    free(x_str);
    free(f_str);
}

void ir2asm_16(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // PARAM x
    printf("TODO: ir2asm func %d\n", 16);
}

void ir2asm_17(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // READ x

    printf("TODO: ir2asm func %d\n", 17);
}

void ir2asm_18(char *str, size_t nmatch, regmatch_t *pmatch)
{
    // WRITE x
    printf("TODO: ir2asm func %d\n", 18);
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
