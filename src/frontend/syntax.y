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

%left PLUS MINUS
%left STAR DIV

%start Program

%%
Program:
    Exp
    ;

Exp:
    Exp PLUS Exp
    | Exp MINUS Exp
    | Exp STAR Exp
    | Exp DIV Exp
    | ID
    | INT
    | FLOAT
    ;
%%

void yyerror(const char *message) {
    fprintf(stderr, "Error type B at Line %d: %s\n",
        yylineno, message);
}