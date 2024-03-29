%{
  #include <stdio.h>
  #include "typedef.h"
  #include "lex.yy.c"
  node_t* create_tree_node(const char* name,const int first_line,node_t* children[],int child_num){
        node_t* temp = malloc(sizeof(node_t));
        memset(temp,0,sizeof(node_t));
        temp->name = strdup(name);
        temp->first_line = first_line;
        temp->is_terminal = 0;
        for(int i = 0; i < child_num; ++i){
            temp->children[i] = children[i];
        }
        return temp;
  }
  void set_error();
%}
%locations
%union {
  node_t* node_ptr;
}

%token <node_ptr> INT
%token <node_ptr> FLOAT
%token <node_ptr> ID
%token <node_ptr> TYPE
%token <node_ptr> RELOP
%token <node_ptr> SEMI COMMA ASSIGNOP PLUS MINUS STAR DIV AND OR DOT NOT LP RP LB RB LC RC STRUCT RETURN IF ELSE WHILE
%type <node_ptr> Program ExtDefList ExtDef ExtDecList
%type <node_ptr> Specifier StructSpecifier OptTag Tag
%type <node_ptr> VarDec FunDec VarList ParamDec
%type <node_ptr> CompSt StmtList Stmt
%type <node_ptr> DefList Def DecList Dec
%type <node_ptr> Exp Args

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NEG NOT
%left LP RP LB RB DOT
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%start Program

%%
// High-level Definitions
Program: ExtDefList {node_t* children[1] = {$1}; $$ = create_tree_node("Program",@1.first_line,children,1); root = $$;}
| error {}
;
ExtDefList: ExtDef ExtDefList {node_t* children[2] = {$1,$2}; $$ = create_tree_node("ExtDefList",@1.first_line,children,2);}
| {$$ = NULL;}
| error {}
;
ExtDef: Specifier ExtDecList SEMI {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("ExtDef",@1.first_line,children,3);
}
| Specifier SEMI {node_t* children[2] = {$1,$2}; $$ = create_tree_node("ExtDef",@1.first_line,children,2);}
| Specifier FunDec CompSt {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("ExtDef",@1.first_line,children,3);}
| error {}
;
ExtDecList: VarDec {node_t* children[1] = {$1}; $$ = create_tree_node("ExtDecList",@1.first_line,children,1);}
| VarDec COMMA ExtDecList {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("ExtDecList",@1.first_line,children,3);}
| error {}
;
// Specifiers
Specifier: TYPE {node_t* children[1] = {$1}; $$ = create_tree_node("Specifier",@1.first_line,children,1);}
| StructSpecifier {node_t* children[1] = {$1}; $$ = create_tree_node("Specifier",@1.first_line,children,1);}
| error ID {}
;
StructSpecifier: STRUCT OptTag LC DefList RC {node_t* children[5] = {$1,$2,$3,$4,$5}; $$ = create_tree_node("StructSpecifier",@1.first_line,children,5);}
| STRUCT Tag {node_t* children[2] = {$1,$2}; $$ = create_tree_node("StructSpecifier",@1.first_line,children,2);}
| error RC {}
;
OptTag: ID {node_t* children[1] = {$1}; $$ = create_tree_node("OptTag",@1.first_line,children,1);}
| {$$ = NULL;}
| error {}
;
Tag: ID {node_t* children[1] = {$1}; $$ = create_tree_node("Tag",@1.first_line,children,1);}
| error INT ID {}
;
// Declarators
VarDec: ID {node_t* children[1] = {$1}; $$ = create_tree_node("VarDec",@1.first_line,children,1);}
| VarDec LB INT RB {node_t* children[4] = {$1,$2,$3,$4}; $$ = create_tree_node("VarDec",@1.first_line,children,4);}
| error INT ID {}
| error RB {}
;
FunDec: ID LP VarList RP {node_t* children[4] = {$1,$2,$3,$4}; $$ = create_tree_node("FunDec",@1.first_line,children,4);}
| ID LP RP {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("FunDec",@1.first_line,children,3);}
| error RP {}
;
VarList: ParamDec COMMA VarList {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("VarList",@1.first_line,children,3);}
| ParamDec {node_t* children[1] = {$1}; $$ = create_tree_node("VarList",@1.first_line,children,1);}
| error {}
;
ParamDec: Specifier VarDec {node_t* children[2] = {$1,$2}; $$ = create_tree_node("ParamDec",@1.first_line,children,2);}
| error {}
;
// Statements
CompSt: LC DefList StmtList RC {node_t* children[4] = {$1,$2,$3,$4}; $$ = create_tree_node("CompSt",@1.first_line,children,4);}
| error RC {}
;
StmtList: Stmt StmtList {node_t* children[2] = {$1,$2}; $$ = create_tree_node("StmtList",@1.first_line,children,2);}
| {$$ = NULL;}
| error {}
;
Stmt: Exp SEMI {node_t* children[2] = {$1,$2}; $$ = create_tree_node("Stmt",@1.first_line,children,2);}
| CompSt {node_t* children[1] = {$1}; $$ = create_tree_node("Stmt",@1.first_line,children,1);}
| RETURN Exp SEMI {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Stmt",@1.first_line,children,3);}
| IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {node_t* children[5] = {$1,$2,$3,$4,$5}; $$ = create_tree_node("Stmt",@1.first_line,children,5);}
| IF LP Exp RP Stmt ELSE Stmt {node_t* children[7] = {$1,$2,$3,$4,$5,$6,$7}; $$ = create_tree_node("Stmt",@1.first_line,children,7);}
| WHILE LP Exp RP Stmt {node_t* children[5] = {$1,$2,$3,$4,$5}; $$ = create_tree_node("Stmt",@1.first_line,children,5);}
| error SEMI {}
;
// Local Definitions
DefList: Def DefList {node_t* children[2] = {$1,$2}; $$ = create_tree_node("DefList",@1.first_line,children,2);}
| {$$ = NULL;}
| error {}
;
Def: Specifier DecList SEMI {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Def",@1.first_line,children,3);}
| error SEMI {}
;
DecList: Dec {node_t* children[1] = {$1}; $$ = create_tree_node("DecList",@1.first_line,children,1);}
| Dec COMMA DecList {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("DecList",@1.first_line,children,3);}
| error {}
;
Dec: VarDec {node_t* children[1] = {$1}; $$ = create_tree_node("Dec",@1.first_line,children,1);}
| VarDec ASSIGNOP Exp {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Dec",@1.first_line,children,3);}
| error {}
;
// Expressions
Exp: Exp ASSIGNOP Exp {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Exp",@1.first_line,children,3);}
| Exp AND Exp {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Exp",@1.first_line,children,3);}
| Exp OR Exp {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Exp",@1.first_line,children,3);}
| Exp RELOP Exp {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Exp",@1.first_line,children,3);}
| Exp PLUS Exp {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Exp",@1.first_line,children,3);}
| Exp MINUS Exp {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Exp",@1.first_line,children,3);}
| Exp STAR Exp {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Exp",@1.first_line,children,3);}
| Exp DIV Exp {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Exp",@1.first_line,children,3);}
| LP Exp RP {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Exp",@1.first_line,children,3);}
| MINUS Exp %prec NEG {node_t* children[2] = {$1,$2}; $$ = create_tree_node("Exp",@1.first_line,children,2);}
| NOT Exp {node_t* children[2] = {$1,$2}; $$ = create_tree_node("Exp",@1.first_line,children,2);}
| ID LP Args RP {node_t* children[4] = {$1,$2,$3,$4}; $$ = create_tree_node("Exp",@1.first_line,children,4);}
| ID LP RP {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Exp",@1.first_line,children,3);}
| Exp LB Exp RB {node_t* children[4] = {$1,$2,$3,$4}; $$ = create_tree_node("Exp",@1.first_line,children,4);}
| Exp DOT ID {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Exp",@1.first_line,children,3);}
| ID {node_t* children[1] = {$1}; $$ = create_tree_node("Exp",@1.first_line,children,1);}
| INT {node_t* children[1] = {$1}; $$ = create_tree_node("Exp",@1.first_line,children,1);}
| FLOAT {node_t* children[1] = {$1}; $$ = create_tree_node("Exp",@1.first_line,children,1);}
| error {}
;
Args: Exp COMMA Args {node_t* children[3] = {$1,$2,$3}; $$ = create_tree_node("Args",@1.first_line,children,3);}
| Exp {node_t* children[1] = {$1}; $$ = create_tree_node("Args",@1.first_line,children,1);}
| error {}
;
%%
yyerror(char* msg) {
  set_error();
  printf("Error type B at Line %d: %s\n", yylineno, msg);
}