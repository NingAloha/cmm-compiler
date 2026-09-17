%{
#include <stdio.h>

int yylex(void);
extern int yylineno;
void yyerror(const char *message);
%}

%token INT FLOAT ID TYPE
%token SEMI COMMA ASSIGNOP RELOP
%token PLUS MINUS STAR DIV AND OR DOT NOT
%token LP RP LB RB LC RC
%token STRUCT RETURN IF ELSE WHILE

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT UMINUS
%left LP RP LB RB DOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%start Program

%%
Program:
    ExtDefList
    ;

ExtDefList:
    ExtDef ExtDefList
    | /* empty */
    ;

ExtDef:
    Specifier ExtDecList SEMI
    | Specifier SEMI
    | Specifier FunDec CompSt
    ;

ExtDecList:
    VarDec
    | VarDec COMMA ExtDecList
    ;

FunDec:
    ID LP VarList RP
    | ID LP RP
    ;

VarList:
    ParamDec COMMA VarList
    | ParamDec
    ;

ParamDec:
    Specifier VarDec
    ;

Stmt:
    Exp SEMI
    | RETURN Exp SEMI
    | CompSt
    | WHILE LP Exp RP Stmt
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
    | IF LP Exp RP Stmt ELSE Stmt
    ;

CompSt:
    LC DefList StmtList RC
    ;

StmtList:
    Stmt StmtList
    | /* empty */
    ;

DefList:
    Def DefList
    | /* empty */
    ;

Def:
    Specifier DecList SEMI
    ;

DecList:
    Dec
    | Dec COMMA DecList
    ;

Dec:
    VarDec
    | VarDec ASSIGNOP Exp
    ;

VarDec:
    ID
    | VarDec LB INT RB
    ;

Specifier:
    TYPE
    | STRUCT OptTag LC DefList RC
    | STRUCT Tag
    ;

Tag:
    ID
    ;

OptTag:
    ID
    | /* empty */
    ;

Exp:
    ID
    | INT
    | FLOAT
    | Exp PLUS Exp
    | Exp MINUS Exp
    | Exp STAR Exp
    | Exp DIV Exp
    | LP Exp RP
    | MINUS Exp %prec UMINUS
    | NOT Exp
    | Exp RELOP Exp
    | Exp AND Exp
    | Exp OR Exp
    | Exp ASSIGNOP Exp
    | ID LP RP
    | ID LP Args RP
    | Exp LB Exp RB
    | Exp DOT ID
    ;

Args:
    Exp COMMA Args
    | Exp
    ;
%%

void yyerror(const char *message) {
    fprintf(stderr, "Error type B at Line %d: %s\n",
        yylineno, message);
}

#include "lex.yy.c"