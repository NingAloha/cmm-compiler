%option noyywrap yylineno noinput nounput
%x COMMENT

%{
#include <stdio.h>
#include "syntax.h"
static int comment_start_line;
%}

DIGIT [0-9]
ID [A-Za-z_][A-Za-z0-9_]*
FLOAT {DIGIT}+\.{DIGIT}+

%%

"int" { printf("TYPE: %s\n", yytext); }
"float" { printf("TYPE: %s\n", yytext); }

"struct" { printf("STRUCT\n"); }
"return" { printf("RETURN\n"); }
"if" { printf("IF\n"); }
"else" { printf("ELSE\n"); }
"while" { printf("WHILE\n"); }

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

">=" { return RELOP; }
"<=" { return RELOP; }
"==" { return RELOP; }
"!=" { return RELOP; }

"&&" { return AND; }
"||" { return OR; }

";" { printf("SEMI\n"); }
"," { printf("COMMA\n"); }
"(" { return LP; }
")" { return RP; }
"[" { printf("LB\n"); }
"]" { printf("RB\n"); }
"{" { printf("LC\n"); }
"}" { printf("RC\n"); }
"=" { return ASSIGNOP; }
"+" { return PLUS; }
"-" { return MINUS; }
"*" { return STAR; }
"/" { return DIV; }
"." { printf("DOT\n"); }
"!" { return NOT; }

">"  { return RELOP; }
"<"  { return RELOP; }

{FLOAT} { return FLOAT; }
{DIGIT}+ { return INT; }
{ID} { return ID; }

. {
    fprintf(stderr,
        "Error type A at Line %d: Mysterious character \"%s\".\n",
        yylineno, yytext);
}

%%