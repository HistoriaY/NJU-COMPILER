#include "ir_pattern.h"
#include <stdlib.h>
#include <stdio.h>

// "[a-zA-Z_]\\w*" simplify to "\\w+" there is no worry to trust in ir.c that no number at the beginning
// !!!maybe exist RE conflict, need to check!!!
char *ir_pattern_str[IR_PATTERN_NUM] = {
    "^LABEL (\\w+) :$",                                   // LABEL x :
    "^FUNCTION (\\w+) :$",                                // FUNCTION f :
    "^(\\w+) := (#?\\w+)$",                               // x := y
    "^(\\w+) := (#?\\w+) \\+ (#?\\w+)$",                  // x := y + z
    "^(\\w+) := (#?\\w+) - (#?\\w+)$",                    // x := y - z
    "^(\\w+) := (#?\\w+) \\* (#?\\w+)$",                  // x := y * z
    "^(\\w+) := (#?\\w+) / (#?\\w+)$",                    // x := y / z
    "^(\\w+) := &(\\w+)$",                                // x := &y
    "^(\\w+) := \\*(\\w+)$",                              // x := *y
    "^\\*(\\w+) := (#?\\w+)$",                            //*x := y
    "^GOTO (\\w+)$",                                      // GOTO x
    "^IF (\\w+) (>|<|>=|<=|==|!=) (#?\\w+) GOTO (\\w+)$", // IF x [relop] y GOTO z
    "^RETURN (\\w+)$",                                    // RETURN x
    "^DEC (\\w+) ([0-9]+)$",                              // DEC x [size]
    "^ARG (\\w+)$",                                       // ARG x
    "^(\\w+) := CALL (\\w+)$",                            // x := CALL f
    "^PARAM (\\w+)$",                                     // PARAM x
    "^READ (\\w+)$",                                      // READ x
    "^WRITE (\\w+)$"                                      // WRITE x
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
