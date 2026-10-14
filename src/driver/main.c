#include <stdio.h>
#include <string.h>

#include "../frontend/tree.h"
#include "../ir/generator.h"
#include "../semantic/semantic.h"

extern FILE *yyin;
extern TreeNode *syntax_tree_root;
extern int lexical_error;
extern int syntax_error;
int yyparse(void);

int main(int argc, char *argv[]) {
  int tree_mode = 0;
  int ir_mode = 0;
  const char *source_path = NULL;
  const char *ir_path = NULL;

  if (argc == 2) {
    source_path = argv[1];
  } else if (argc == 3 && strcmp(argv[1], "--tree") == 0) {
    tree_mode = 1;
    source_path = argv[2];
  } else if (argc == 3 && strcmp(argv[1], "--ir") != 0) {
    ir_mode = 1;
    source_path = argv[1];
    ir_path = argv[2];
  } else if (argc == 4 && strcmp(argv[1], "--ir") == 0) {
    ir_mode = 1;
    source_path = argv[2];
    ir_path = argv[3];
  } else {
    fprintf(stderr,
            "Usage: %s <source.cmm> [output.ir]\n"
            "       %s --tree <source.cmm>\n"
            "       %s --ir <source.cmm> <output.ir>\n",
            argv[0], argv[0], argv[0]);
    return 1;
  }

  yyin = fopen(source_path, "r");
  if (yyin == NULL) {
    perror(source_path);
    return 1;
  }

  int parse_status = yyparse();
  fclose(yyin);

  int parse_succeeded = parse_status == 0 && !lexical_error && !syntax_error &&
                        syntax_tree_root != NULL;
  int exit_status = 0;

  if (parse_succeeded) {
    if (tree_mode) {
      tree_print(syntax_tree_root, 0);
    } else {
      int semantic_error_count = semantic_analyze(syntax_tree_root);

      if (ir_mode && semantic_error_count == 0) {
        FILE *ir_output = fopen(ir_path, "w");

        if (ir_output == NULL) {
          perror(ir_path);
          exit_status = 1;
        } else {
          IrGenerator generator;
          ir_generator_init(&generator, ir_output);

          int generated = ir_generate_program(&generator, syntax_tree_root);
          ir_generator_clear(&generator);

          if (!generated) {
            fprintf(stderr, "Failed to generate IR.\n");
            exit_status = 1;
          }

          if (fclose(ir_output) == EOF) {
            perror(ir_path);
            exit_status = 1;
          }
        }
      }
    }
  }

  tree_free(syntax_tree_root);

  return exit_status;
}
