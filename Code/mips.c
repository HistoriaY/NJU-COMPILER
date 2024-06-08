#include "mips.h"
#include <stdlib.h>
#include <stdio.h>

static char *asm_output_path;

mips_reg_t reg(const char *const var)
{
    return r_error;
}

#define IR_PATTERN_NUM 19

// "[a-zA-Z_]\\w*" simplify to "\\w+" there is no worry to trust in ir.c that no number at the beginning
char *ir_pattern_str[IR_PATTERN_NUM] = {
    "LABEL (\\w+) :",                                 // LABEL x :
    "FUNCTION (\\w+) :",                              // FUNCTION f :
    "(\\w+) := (#?\\w+)",                             // x := y
    "(\\w+) := (#?\\w+) + (#?\\w+)",                  // x := y + z
    "(\\w+) := (#?\\w+) - (#?\\w+)",                  // x := y - z
    "(\\w+) := (#?\\w+) * (#?\\w+)",                  // x := y * z
    "(\\w+) := (#?\\w+) / (#?\\w+)",                  // x := y / z
    "(\\w+) := &(\\w+)",                              // x := &y
    "(\\w+) := \\*(\\w+)",                            // x := *y
    "\\*(\\w+) := (#?\\w+)",                          //*x := y
    "GOTO (\\w+)",                                    // GOTO x
    "IF (\\w+) (>|<|>=|<=|==|!=) (\\w+) GOTO (\\w+)", // IF x [relop] y GOTO z
    "RETURN (\\w+)",                                  // RETURN x
    "DEC (\\w+) ([0-9]+)",                            // DEC x [size]
    "ARG (\\w+)",                                     // ARG x
    "(\\w+) := CALL (\\w+)",                          // x := CALL f
    "PARAM (\\w+)",                                   // PARAM x
    "READ (\\w+)",                                    // READ x
    "WRITE (\\w+)"                                    // WRITE X
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

#define pmatch_len(i) pmatch[i].rm_eo - pmatch[i].rm_so
#define pmatch_str(i) str + pmatch[i].rm_so

void ir2asm_0(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 0);
}

void ir2asm_1(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 1);
}

void ir2asm_2(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 2);
}

void ir2asm_3(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 3);
}

void ir2asm_4(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 4);
}

void ir2asm_5(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 5);
}

void ir2asm_6(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 6);
}

void ir2asm_7(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 7);
}

void ir2asm_8(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 8);
}

void ir2asm_9(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 9);
}

void ir2asm_10(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 10);
}

void ir2asm_11(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 11);
}

void ir2asm_12(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 12);
}

void ir2asm_13(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 13);
}

void ir2asm_14(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 14);
}

void ir2asm_15(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 15);
}

void ir2asm_16(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 16);
}

void ir2asm_17(char *str, size_t nmatch, regmatch_t *pmatch)
{
    printf("TODO: ir2asm func %d\n", 17);
}

void ir2asm_18(char *str, size_t nmatch, regmatch_t *pmatch)
{
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
    asm_output_path = asm_file;
    FILE *file = fopen(asm_output_path, "w");
    if (file == NULL)
    {
        fprintf(stderr, "can't open asm file: %s\n", asm_output_path);
        exit(2);
    }
    trans_all_ir();
    fclose(file);
}
