#include <stdio.h>
#include <string.h>
#include "typedef.h"
#include "semantics.h"
#include "ir.h"
#include "mips.h"
#include "str_func.h"

static int error = 0;

void set_error()
{
    error = 1;
}

void print_node_info(node_t *node, int indent)
{
    // print indent
    for (int i = 0; i < indent; ++i)
        printf("  ");

    // judge is_terminal
    if (node->is_terminal == 0)
    {
        // non_terminal
        printf("%s (%d)\n", node->name, node->first_line);
        // children node
        for (int i = 0; i < 7; ++i)
            if (node->children[i] != NULL)
                print_node_info(node->children[i], indent + 1);
    }
    else
    {
        // terminal
        printf("%s", node->name);
        if (strcmp(node->name, "ID") == 0)
            printf(": %s\n", node->tev.id);
        else if (strcmp(node->name, "TYPE") == 0)
        {
            if (node->tev.type == TYPE_token_INT)
                printf(": int\n");
            else if (node->tev.type == TYPE_token_FLOAT)
                printf(": float\n");
        }
        else if (strcmp(node->name, "INT") == 0)
            printf(": %d\n", node->tev.int_val);
        else if (strcmp(node->name, "FLOAT") == 0)
            printf(": %f\n", node->tev.float_val);
        else
            printf("\n");
    }
}

void yyrestart(FILE *input_file);
int yyparse(void);

int main(int argc, char **argv)
{
    int lab = 5;
    // lab1: ./parser test.cmm
    if (lab == 1)
    {
        FILE *f = fopen(argv[1], "r");
        if (!f)
        {
            perror(argv[1]);
            return 1;
        }
        yyrestart(f);
        yyparse();
        if (!error)
            print_node_info(root, 0);
        fclose(f);
        return 0;
    }
    // lab2: ./parser test.cmm
    else if (lab == 2)
    {
        FILE *f = fopen(argv[1], "r");
        if (!f)
        {
            perror(argv[1]);
            return 1;
        }
        yyrestart(f);
        yyparse();
        if (!error)
            semantic_analysis(root);
        fclose(f);
        return 0;
    }
    // lab3: ./parser test.cmm out.ir
    else if (lab == 3)
    {
        FILE *f = fopen(argv[1], "r");
        if (!f)
        {
            perror(argv[1]);
            return 1;
        }
        yyrestart(f);
        yyparse();
        if (!error)
            semantic_analysis(root);
        if (!error)
        {
            trans_Program2ir(root);
            output_ir_codes(argv[2]);
        }
        fclose(f);
        return 0;
    }
    // lab4: ./parser test.cmm out.s
    else if (lab == 4)
    {
        FILE *f = fopen(argv[1], "r");
        if (!f)
        {
            perror(argv[1]);
            return 1;
        }
        yyrestart(f);
        yyparse();
        if (!error)
            semantic_analysis(root);
        if (!error)
        {
            trans_Program2ir(root);
            // output_ir_codes("out.ir");
            trans_ir2asm(argv[2]);
        }
        fclose(f);
        return 0;
    }
    // lab5: ./parser in.ir out.ir
    else if (lab == 5)
    {
        FILE *in_ir = fopen(argv[1], "r");
        if (!in_ir)
        {
            perror(argv[1]);
            return 1;
        }
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), in_ir))
        {
            ir_code_t *tmp = new_empty_code();
            int len = strlen(buffer);
            if (buffer[len - 1] == '\n')
                buffer[len - 1] = '\0';
            tmp->code_str = strdup(buffer);
            ir_start = merge_code(2, ir_start, tmp);
        }
        output_ir_codes(argv[2]);
        fclose(in_ir);
        return 0;
    }
    return 1;
}