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

%start Program

%%
Program:
    Exp
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