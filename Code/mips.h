#ifndef MIPS_H
#define MIPS_H
#include "ir.h"
#include "typedef.h"

char *createFormattedString(const char *format, ...);

#define REG_NUM 33
typedef enum mips_reg
{
    reg_zero,
    reg_at,
    reg_v0,
    reg_v1,
    reg_a0,
    reg_a1,
    reg_a2,
    reg_a3,
    reg_t0,
    reg_t1,
    reg_t2,
    reg_t3,
    reg_t4,
    reg_t5,
    reg_t6,
    reg_t7,
    reg_s0,
    reg_s1,
    reg_s2,
    reg_s3,
    reg_s4,
    reg_s5,
    reg_s6,
    reg_s7,
    reg_t8,
    reg_t9,
    reg_k0,
    reg_k1,
    reg_gp,
    reg_sp,
    reg_fp,
    reg_ra,
    reg_null
} mips_reg_t;
mips_reg_t reg_no(char *var);

// typedef struct reg_descriptor_s
// {
//     char *var;
// } reg_descriptor_t;

typedef struct addr_descriptor_s
{
    mips_reg_t reg;
    int bias;
} addr_descriptor_t;

void init_ad_table();
addr_descriptor_t *look_up_ad(char *var);
void insert_ad(char *var, addr_descriptor_t *ad);

void init_ir_pattern();

void trans_all_ir();

void trans_ir2asm(char *asm_file);

#endif
