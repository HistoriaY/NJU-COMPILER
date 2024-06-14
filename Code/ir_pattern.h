#ifndef IR_PATTERN_H
#define IR_PATTERN_H
#include <regex.h>

#define IR_PATTERN_NUM 19

extern regex_t ir_pattern[IR_PATTERN_NUM];

void init_ir_pattern();

#endif
