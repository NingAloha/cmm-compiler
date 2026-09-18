%{
#include <stdio.h>
#include "tree.h"

int yylex(void);
extern int yylineno;
extern int lexical_error;
int syntax_error = 0;
void yyerror(const char *message);
TreeNode *syntax_tree_root;
%}

%union {
    TreeNode *node;
}

%token <node> INT FLOAT ID TYPE
%token <node> SEMI COMMA ASSIGNOP RELOP
%token <node> PLUS MINUS STAR DIV AND OR DOT NOT
%token <node> LP RP LB RB LC RC
%token <node> STRUCT RETURN IF ELSE WHILE

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

%type <node> Program ExtDefList ExtDef ExtDecList
%type <node> FunDec VarList ParamDec
%type <node> Stmt CompSt StmtList
%type <node> DefList Def DecList Dec VarDec
%type <node> Specifier StructSpecifier Tag OptTag
%type <node> Exp Args

%start Program

%%
Program:
    ExtDefList {
        $$ = tree_new("Program", NULL,
            $1 != NULL ? $1->line : yylineno);
        tree_add_child($$, $1);
        syntax_tree_root = $$;
    }
    ;

ExtDefList:
    ExtDef ExtDefList {
        $$ = tree_new("ExtDefList", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
    }
    | /* empty */ {
        $$ = NULL;
    }
    ;

ExtDef:
    Specifier ExtDecList SEMI {
        $$ = tree_new("ExtDef", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
    }
    | Specifier SEMI {
        $$ = tree_new("ExtDef", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
    }
    | Specifier FunDec CompSt {
        $$ = tree_new("ExtDef", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
    }
    | error SEMI {
        yyerrok;
        $$ = tree_new("ExtDef", NULL, yylineno);
    }
    ;

ExtDecList:
    VarDec {
        $$ = tree_new("ExtDecList", NULL, $1->line);
        tree_add_child($$, $1);
    }
    | VarDec COMMA ExtDecList {
        $$ = tree_new("ExtDecList", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
    }
    ;

FunDec:
    ID LP VarList RP {
        $$ = tree_new("FunDec", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
        tree_add_child($$, $4);
    }
    | ID LP RP {
        $$ = tree_new("FunDec", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
    }
    | ID LP error RP {
        yyerrok;
        $$ = tree_new("FunDec", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $4);
    }
    ;

VarList:
    ParamDec COMMA VarList {
        $$ = tree_new("VarList", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
    }
    | ParamDec {
        $$ = tree_new("VarList", NULL, $1->line);
        tree_add_child($$, $1);
    }
    ;

ParamDec:
    Specifier VarDec {
        $$ = tree_new("ParamDec", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
    }
    ;

Stmt:
    Exp SEMI {
        $$ = tree_new("Stmt", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
    }
    | RETURN Exp SEMI {
        $$ = tree_new("Stmt", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
    }
    | CompSt {
        $$ = tree_new("Stmt", NULL, $1->line);
        tree_add_child($$, $1);
    }
    | WHILE LP Exp RP Stmt {
        $$ = tree_new("Stmt", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
        tree_add_child($$, $4);
        tree_add_child($$, $5);
    }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
        $$ = tree_new("Stmt", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
        tree_add_child($$, $4);
        tree_add_child($$, $5);
    }
    | IF LP Exp RP Stmt ELSE Stmt {
        $$ = tree_new("Stmt", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
        tree_add_child($$, $4);
        tree_add_child($$, $5);
        tree_add_child($$, $6);
        tree_add_child($$, $7);
    }
    | error SEMI {
        $$ = tree_new("Stmt", NULL, yylineno);
    }
    | WHILE LP error RP Stmt {
        yyerrok;
        $$ = tree_new("Stmt", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $4);
        tree_add_child($$, $5);
    }
    | IF LP error RP Stmt %prec LOWER_THAN_ELSE {
        yyerrok;
        $$ = tree_new("Stmt", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $4);
        tree_add_child($$, $5);
    }
    | IF LP error RP Stmt ELSE Stmt {
        yyerrok;
        $$ = tree_new("Stmt", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $4);
        tree_add_child($$, $5);
        tree_add_child($$, $6);
        tree_add_child($$, $7);
    }
    ;

CompSt:
    LC DefList StmtList RC {
        $$ = tree_new("CompSt", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
        tree_add_child($$, $4);
    }
    | LC DefList error RC {
        yyerrok;
        $$ = tree_new("CompSt", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $4);
    }
    ;

StmtList:
   Stmt StmtList {
        $$ = tree_new("StmtList", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
    }
    | /* empty */ {
        $$ = NULL;
    }
    ;

DefList:
    Def DefList {
        $$ = tree_new("DefList", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
    }
    | /* empty */ {
        $$ = NULL;
    }
    ;

Def:
    Specifier DecList SEMI {
        $$ = tree_new("Def", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
    }
    | Specifier error SEMI {
        yyerrok;
        $$ = tree_new("Def", NULL, $1->line);
        tree_add_child($$, $1);
    }
    ;

DecList:
    Dec {
        $$ = tree_new("DecList", NULL, $1->line);
        tree_add_child($$, $1);
    }
    | Dec COMMA DecList {
        $$ = tree_new("DecList", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
    }
    ;

Dec:
    VarDec {
        $$ = tree_new("Dec", NULL, $1->line);
        tree_add_child($$, $1);
    }
    | VarDec ASSIGNOP Exp {
        $$ = tree_new("Dec", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
    }
    ;

VarDec:
    ID {
        $$ = tree_new("VarDec", NULL, $1->line);
        tree_add_child($$, $1);
    }
    | VarDec LB INT RB {
        $$ = tree_new("VarDec", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
        tree_add_child($$, $4);
    }
    | VarDec LB error RB {
        yyerrok;
        $$ = tree_new("VarDec", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $4);
    }
    ;

Specifier:
    TYPE {
        $$ = tree_new("Specifier", NULL, $1->line);
        tree_add_child($$, $1);
    }
    | StructSpecifier {
        $$ = tree_new("Specifier", NULL, $1->line);
        tree_add_child($$, $1);
    }
    ;

StructSpecifier:
    STRUCT OptTag LC DefList RC {
        $$ = tree_new("StructSpecifier", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
        tree_add_child($$, $4);
        tree_add_child($$, $5);
    }
    | STRUCT Tag {
        $$ = tree_new("StructSpecifier", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
    }
    | STRUCT OptTag LC DefList error RC {
        yyerrok;
        $$ = tree_new("StructSpecifier", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
        tree_add_child($$, $4);
        tree_add_child($$, $6);
    }
    ;

Tag:
    ID {
        $$ = tree_new("Tag", NULL, $1->line);
        tree_add_child($$, $1);
    }
    ;

OptTag:
    ID {
        $$ = tree_new("OptTag", NULL, $1->line);
        tree_add_child($$, $1);
    }
    | /* empty */ {
        $$ = NULL;
    }
    ;

Exp:
    ID {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
      }
    | INT {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
      }
    | FLOAT {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
      }
    | Exp PLUS Exp {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
      }
    | Exp MINUS Exp {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
      }
    | Exp STAR Exp {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
      }
    | Exp DIV Exp {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
      }
    | LP Exp RP {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
      }
    | MINUS Exp %prec UMINUS {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
      }
    | NOT Exp {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
      }
    | Exp RELOP Exp {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
      }
    | Exp AND Exp {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
      }
    | Exp OR Exp {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
      }
    | Exp ASSIGNOP Exp {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
      }
    | ID LP RP {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
      }
    | ID LP Args RP {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
          tree_add_child($$, $4);
      }
    | Exp LB Exp RB {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
          tree_add_child($$, $4);
      }
    | Exp DOT ID {
          $$ = tree_new("Exp", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
      }
    | LP error RP {
        yyerrok;
        $$ = tree_new("Exp", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $3);
    }
    | ID LP error RP {
        yyerrok;
        $$ = tree_new("Exp", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $4);
    }
    | Exp LB error RB {
        yyerrok;
        $$ = tree_new("Exp", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $4);
    }
    ;

Args:
    Exp COMMA Args {
          $$ = tree_new("Args", NULL, $1->line);
          tree_add_child($$, $1);
          tree_add_child($$, $2);
          tree_add_child($$, $3);
      }
    | Exp {
          $$ = tree_new("Args", NULL, $1->line);
          tree_add_child($$, $1);
      }
    ;
%%

void yyerror(const char *message) {
    (void)message;

    if (lexical_error) {
        return;
    }

    syntax_error = 1;
    fprintf(stdout, "Error type B at Line %d: Syntax error.\n", yylineno);
}

#include "lex.yy.c"
