#ifndef MIPS_H
#define MIPS_H
#include <regex.h>
#include "ir.h"

char *createFormattedString(const char *format, ...);

typedef enum mips_reg
{
    r_zero,
    r_at,
    r_v0,
    r_v1,
    r_a0,
    r_a1,
    r_a2,
    r_a3,
    r_t0,
    r_t1,
    r_t2,
    r_t3,
    r_t4,
    r_t5,
    r_t6,
    r_t7,
    r_s0,
    r_s1,
    r_s2,
    r_s3,
    r_s4,
    r_s5,
    r_s6,
    r_s7,
    r_t8,
    r_t9,
    r_k0,
    r_k1,
    r_gp,
    r_sp,
    r_fp,
    r_ra,
    r_error
} mips_reg_t;

mips_reg_t reg(const char *const var);

void init_ir_pattern();

void trans_all_ir();

void trans_ir2asm(char *asm_file);

#endif
