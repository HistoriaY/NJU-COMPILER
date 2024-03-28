%{
#include <stdio.h>
#include "typedef.h" 
#include "syntax.tab.h"
    int yycolumn = 1;
#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

    node_t* create_token_node(const char* name,const int first_line,const token_extra_value_t tev){
        node_t* temp = malloc(sizeof(node_t));
        memset(temp,0,sizeof(node_t));
        temp->name = strdup(name);
        temp->first_line = first_line;
        temp->is_terminal = 1;
        temp->tev = tev;
        return temp;
    }
    void set_error();
%}
%option yylineno
WHITESPACE [ \r\t]
INT [1-9][0-9]*|0[0-7]*|0[xX][0-9a-fA-F]+
FLOAT [0-9]+\.[0-9]+
ID [a-zA-Z_][a-zA-Z0-9_]*
%%
"int" {
    token_extra_value_t tev;
    tev.type = type_INT;
    yylval.node_ptr = create_token_node("TYPE",yylineno,tev);
    return TYPE;
}
"float" {
    token_extra_value_t tev;
    tev.type = type_FLOAT;
    yylval.node_ptr = create_token_node("TYPE",yylineno,tev);
    return TYPE;
}
"struct" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("STRUCT",yylineno,tev);
    return STRUCT;
}
"return" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("RETURN",yylineno,tev);
    return RETURN;
}
"if" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("IF",yylineno,tev);
    return IF;
}
"else" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("ELSE",yylineno,tev);
    return ELSE;
}
"while" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("WHILE",yylineno,tev);
    return WHILE;
}
";" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("SEMI",yylineno,tev);
    return SEMI;
}
"," {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("COMMA",yylineno,tev);
    return COMMA;
}
"=" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("ASSIGNOP",yylineno,tev);
    return ASSIGNOP;
}
">" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("RELOP",yylineno,tev);
    return RELOP;
}
"<" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("RELOP",yylineno,tev);
    return RELOP;
}
">=" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("RELOP",yylineno,tev);
    return RELOP;
}
"<=" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("RELOP",yylineno,tev);
    return RELOP;
}
"==" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("RELOP",yylineno,tev);
    return RELOP;
}
"!=" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("RELOP",yylineno,tev);
    return RELOP;
}
"+" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("PLUS",yylineno,tev);
    return PLUS;
}
"-" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("MINUS",yylineno,tev);
    return MINUS;
}
"*" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("STAR",yylineno,tev);
    return STAR;
}
"/" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("DIV",yylineno,tev);
    return DIV;
}
"&&" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("AND",yylineno,tev);
    return AND;
}
"||" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("OR",yylineno,tev);
    return OR;
}
"." {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("DOT",yylineno,tev);
    return DOT;
}
"!" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("NOT",yylineno,tev);
    return NOT;
}
"(" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("LP",yylineno,tev);
    return LP;
}
")" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("RP",yylineno,tev);
    return RP;
}
"[" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("LB",yylineno,tev);
    return LB;
}
"]" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("RB",yylineno,tev);
    return RB;
}
"{" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("LC",yylineno,tev);
    return LC;
}
"}" {
    token_extra_value_t tev;
    yylval.node_ptr = create_token_node("RC",yylineno,tev);
    return RC;
}
{WHITESPACE}+ {}
\n { yycolumn = 1; }
{INT} {
    token_extra_value_t tev;
    char *endptr;
    tev.int_val = strtol(yytext, &endptr, 0);
    yylval.node_ptr = create_token_node("INT",yylineno,tev);
    return INT;
}
{FLOAT} {
    token_extra_value_t tev;
    tev.float_val = atof(yytext);
    yylval.node_ptr = create_token_node("FLOAT",yylineno,tev);
    return FLOAT;
}
{ID} {
    token_extra_value_t tev;
    tev.id = strdup(yytext);
    yylval.node_ptr = create_token_node("ID",yylineno,tev);
    return ID;
}
. { set_error(); printf("Error type A at Line %d: Mysterious characters \'%s\'\n",yylineno, yytext); }
%%