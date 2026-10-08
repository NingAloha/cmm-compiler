#include <stdio.h>

#include "../frontend/tree.h"
#include "../semantic/semantic.h"

extern FILE *yyin;
extern TreeNode *syntax_tree_root;
extern int lexical_error;
extern int syntax_error;
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

    int parse_status = yyparse();
    fclose(yyin);

    int semantic_errors = 0;

    if (parse_status == 0 && !lexical_error && !syntax_error &&
        syntax_tree_root != NULL) {
        tree_print(syntax_tree_root, 0);
        semantic_errors = semantic_analyze(syntax_tree_root);
    }

    tree_free(syntax_tree_root);

    return (parse_status != 0 || lexical_error || syntax_error ||
            semantic_errors != 0)
               ? 1
               : 0;
}
