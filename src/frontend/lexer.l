%option noyywrap yylineno noinput nounput
%x COMMENT

%{
#include <stdio.h>
#include "syntax.tab.h"
static int comment_start_line;
%}

DIGIT [0-9]
ID [A-Za-z_][A-Za-z0-9_]*
FLOAT {DIGIT}+\.{DIGIT}+

%%

"int"   { return TYPE; }
"float" { return TYPE; }

"struct" { return STRUCT; }
"return" { return RETURN; }
"if" { return IF; }
"else" { return ELSE; }
"while" { return WHILE; }

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

";" { return SEMI; }
"," { return COMMA; }
"(" { return LP; }
")" { return RP; }
"[" { return LB; }
"]" { return RB; }
"{" { return LC; }
"}" { return RC; }
"=" { return ASSIGNOP; }
"+" { return PLUS; }
"-" { return MINUS; }
"*" { return STAR; }
"/" { return DIV; }
"." { return DOT; }
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