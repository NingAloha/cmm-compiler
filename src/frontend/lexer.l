%option noyywrap yylineno noinput nounput
%x COMMENT

%{
#include <stdio.h>
#include "syntax.tab.h"
#include "tree.h"

static int comment_start_line;
static int make_token(int token, const char *name, const char *text) {
    yylval.node = tree_new(name, text, yylineno);
    return token;
}
%}

DIGIT [0-9]
ID [A-Za-z_][A-Za-z0-9_]*
FLOAT {DIGIT}+\.{DIGIT}+

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

{FLOAT}  { return make_token(FLOAT, "FLOAT", yytext); }
{DIGIT}+ { return make_token(INT, "INT", yytext); }
{ID}     { return make_token(ID, "ID", yytext); }

. {
    fprintf(stderr,
        "Error type A at Line %d: Mysterious character \"%s\".\n",
        yylineno, yytext);
}

%%