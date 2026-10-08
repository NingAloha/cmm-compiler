#include <stdio.h>
#include <string.h>

#include "../frontend/tree.h"
#include "../semantic/semantic.h"

extern FILE *yyin;
extern TreeNode *syntax_tree_root;
extern int lexical_error;
extern int syntax_error;
int yyparse(void);

int main(int argc, char *argv[]) {
    int print_tree = 0;
    const char *source_path = NULL;

    if (argc == 2) {
        source_path = argv[1];
    } else if (argc == 3 && strcmp(argv[1], "--tree") == 0) {
        print_tree = 1;
        source_path = argv[2];
    } else {
        fprintf(stderr, "Usage: %s [--tree] <source.cmm>\n", argv[0]);
        return 1;
    }

    yyin = fopen(source_path, "r");
    if (yyin == NULL) {
        perror(source_path);
        return 1;
    }

    int parse_status = yyparse();
    fclose(yyin);

    if (parse_status == 0 && !lexical_error && !syntax_error &&
        syntax_tree_root != NULL) {
        if (print_tree) {
            tree_print(syntax_tree_root, 0);
        } else {
            (void)semantic_analyze(syntax_tree_root);
        }
    }

    tree_free(syntax_tree_root);

    return 0;
}
