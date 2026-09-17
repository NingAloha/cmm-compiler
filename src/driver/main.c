#include <stdio.h>
#include "tree.h"

extern FILE *yyin;
extern TreeNode *syntax_tree_root;
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

    int result = yyparse();
    fclose(yyin);
    
    if (result == 0 && syntax_tree_root != NULL) {
        tree_print(syntax_tree_root, 0);
    }

    tree_free(syntax_tree_root);
    return result;
}