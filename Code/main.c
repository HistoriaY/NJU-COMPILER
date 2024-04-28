#include <stdio.h>
#include <string.h>
#include "typedef.h"
#include "semantics.h"
#include "ir.h"

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

void gen_ir_codes(char *output)
{
    FILE *file = fopen(output, "w");
    if (file == NULL)
    {
        fprintf(stderr, "can't open ir output file: %s\n", output);
        return;
    }
    code_t *curr = ir_start;
    if (curr)
    {
        // if (curr->code_str)
        fprintf(file, "%s\n", curr->code_str);
        curr = curr->next;
        while (curr != ir_start)
        {
            // if (curr->code_str)
            fprintf(file, "%s\n", curr->code_str);
            curr = curr->next;
        }
    }
    fclose(file);
}

int main(int argc, char **argv)
{
    if (argc <= 2)
        return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f)
    {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    if (!error)
    {
        // print_node_info(root, 0);
        semantic_analysis(root);
    }
    if (!error)
    {
        trans_Program(root);
        gen_ir_codes(argv[2]);
    }
    fclose(f);
    return 0;
}