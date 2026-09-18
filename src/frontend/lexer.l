%option noyywrap yylineno noinput nounput
%x COMMENT

%{
#include <stdio.h>
#include <stdlib.h>
#include "syntax.tab.h"
#include "tree.h"

static int comment_start_line;
int lexical_error = 0;

static int make_token(int token, const char *name, const char *text) {
    yylval.node = tree_new(name, text, yylineno);
    return token;
}

static int make_int_token(const char *text) {
    char decimal[32];
    unsigned long value = strtoul(text, NULL, 0);

    snprintf(decimal, sizeof(decimal), "%lu", value);
    return make_token(INT, "INT", decimal);
}

static int make_float_token(const char *text) {
    char decimal[64];
    float value = strtof(text, NULL);

    snprintf(decimal, sizeof(decimal), "%f", value);
    return make_token(FLOAT, "FLOAT", decimal);
}
%}

DIGIT [0-9]

ID [A-Za-z_][A-Za-z0-9_]*

EXPONENT      [eE][+-]?{DIGIT}+
FRACTION      ({DIGIT}*\.{DIGIT}+|{DIGIT}+\.{DIGIT}*)
FLOAT_LITERAL ({FRACTION}{EXPONENT}?|{DIGIT}+{EXPONENT})
BAD_FLOAT     ({FRACTION}|{DIGIT}+)[eE][+-]?

HEX_INT  0[xX][0-9a-fA-F]+
BAD_HEX 0[xX][0-9A-Za-z]*
OCT_INT 0[0-7]+
DEC_INT [0-9]+

%%

"int"   { return make_token(TYPE, "TYPE", yytext); }
"float" { return make_token(TYPE, "TYPE", yytext); }

"struct" { return make_token(STRUCT, "STRUCT", ""); }
"return" { return make_token(RETURN, "RETURN", ""); }
"if"     { return make_token(IF, "IF", ""); }
"else"   { return make_token(ELSE, "ELSE", ""); }
"while"  { return make_token(WHILE, "WHILE", ""); }

[ \t\r]+ { /* 忽略空格、Tab 和回车。 */ }
\n { /* yylineno 会自动把行号加一。 */ }

"//".* { /* 忽略从 // 到这一行结尾的全部内容。 */ }
"/*" {
    comment_start_line = yylineno;
    BEGIN(COMMENT);
}
<COMMENT>"*/" {
    BEGIN(INITIAL);
}
<COMMENT>\n {
    /* yylineno 自动更新行号。 */
}
<COMMENT>. {
    /* 忽略注释中的普通字符。 */
}
<COMMENT><<EOF>> {
    lexical_error = 1;
    fprintf(stderr,
        "Error type A at Line %d: Unterminated comment.\n",
        comment_start_line);
    return 0;
}

">=" { return make_token(RELOP, "RELOP", yytext); }
"<=" { return make_token(RELOP, "RELOP", yytext); }
"==" { return make_token(RELOP, "RELOP", yytext); }
"!=" { return make_token(RELOP, "RELOP", yytext); }

"&&" { return make_token(AND, "AND", ""); }
"||" { return make_token(OR, "OR", ""); }

";" { return make_token(SEMI, "SEMI", ""); }
"," { return make_token(COMMA, "COMMA", ""); }
"(" { return make_token(LP, "LP", ""); }
")" { return make_token(RP, "RP", ""); }
"[" { return make_token(LB, "LB", ""); }
"]" { return make_token(RB, "RB", ""); }
"{" { return make_token(LC, "LC", ""); }
"}" { return make_token(RC, "RC", ""); }
"=" { return make_token(ASSIGNOP, "ASSIGNOP", ""); }
"+" { return make_token(PLUS, "PLUS", ""); }
"-" { return make_token(MINUS, "MINUS", ""); }
"*" { return make_token(STAR, "STAR", ""); }
"/" { return make_token(DIV, "DIV", ""); }
"." { return make_token(DOT, "DOT", ""); }
"!" { return make_token(NOT, "NOT", ""); }

">"  { return make_token(RELOP, "RELOP", yytext); }
"<"  { return make_token(RELOP, "RELOP", yytext); }

{FLOAT_LITERAL} { return make_float_token(yytext); }
{BAD_FLOAT} {
    lexical_error = 1;
    fprintf(stderr,
        "Error type A at Line %d: Mysterious character \"%s\".\n",
        yylineno, yytext);
}

{HEX_INT} { return make_int_token(yytext); }
{BAD_HEX} {
    lexical_error = 1;
    fprintf(stderr,
        "Error type A at Line %d: Mysterious character \"%s\".\n",
        yylineno, yytext);
}
{OCT_INT} { return make_int_token(yytext); }
{DEC_INT} { return make_int_token(yytext); }

{ID}     { return make_token(ID, "ID", yytext); }

. {
    lexical_error = 1;
    fprintf(stderr,
        "Error type A at Line %d: Mysterious character \"%s\".\n",
        yylineno, yytext);
}

%%