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
    int semantic_enabled = 0;
    const char *source_path = NULL;

    if (argc == 2) {
        source_path = argv[1];
    } else if (argc == 3 && strcmp(argv[1], "--semantic") == 0) {
        semantic_enabled = 1;
        source_path = argv[2];
    } else {
        fprintf(stderr, "Usage: %s [--semantic] <source.cmm>\n", argv[0]);
        return 1;
    }

    yyin = fopen(source_path, "r");
    if (yyin == NULL) {
        perror(source_path);
        return 1;
    }

    int parse_status = yyparse();
    fclose(yyin);

    int semantic_errors = 0;

    if (parse_status == 0 && !lexical_error && !syntax_error &&
        syntax_tree_root != NULL) {
        tree_print(syntax_tree_root, 0);
        if (semantic_enabled) {
            semantic_errors = semantic_analyze(syntax_tree_root);
        }
    }

    tree_free(syntax_tree_root);

    return (parse_status != 0 || lexical_error || syntax_error ||
            semantic_errors != 0)
               ? 1
               : 0;
}
