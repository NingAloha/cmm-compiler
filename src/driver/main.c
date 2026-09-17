#include <stdio.h>

extern FILE *yyin;
int yyparse(void);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source.cmm>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        perror(argv[1]);
        return 1;
    }

    int parse_result = yyparse();

    fclose(yyin);
    return parse_result;
}