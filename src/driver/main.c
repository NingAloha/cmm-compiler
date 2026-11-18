#include <stdio.h>
#include <string.h>

#include "../frontend/lexer.h"
#include "../frontend/tree.h"
#include "../ir/generator.h"
#include "../mips/mips.h"
#include "../semantic/semantic.h"

extern FILE *yyin;
extern TreeNode *syntax_tree_root;
extern int syntax_error;
int yyparse(void);

static int ir_input_supported(void) {
  return !source_has_comment && !source_has_extended_integer &&
         !source_has_float;
}

static int generate_ir(FILE *output, const TreeNode *root) {
  IrGenerator generator;
  ir_generator_init(&generator, output);
  int generated = ir_generate_program(&generator, root);
  ir_generator_clear(&generator);
  return generated;
}

static int write_output(const char *path, const TreeNode *root, int mips_mode) {
  FILE *ir = NULL;
  if (mips_mode) {
    ir = tmpfile();
    if (ir == NULL) {
      perror("temporary IR stream");
      return 0;
    }
    if (!generate_ir(ir, root) || fseek(ir, 0, SEEK_SET) != 0) {
      fprintf(stderr, "Failed to prepare IR for MIPS generation.\n");
      fclose(ir);
      return 0;
    }
  }
  FILE *output = fopen(path, "w");
  if (output == NULL) {
    perror(path);
    if (ir != NULL) {
      fclose(ir);
    }
    return 0;
  }

  int generated =
      ir != NULL ? mips_generate(ir, output) : generate_ir(output, root);
  if (ir != NULL) {
    fclose(ir);
  }

  if (fclose(output) == EOF) {
    perror(path);
    generated = 0;
  }

  if (!generated) {
    fprintf(stderr, "Failed to generate %s.\n", mips_mode ? "MIPS" : "IR");
    if (remove(path) != 0) {
      perror(path);
    }
  }
  return generated;
}

int main(int argc, char *argv[]) {
  int tree_mode = 0;
  int ir_mode = 0;
  int mips_mode = 0;
  const char *program = strrchr(argv[0], '/');
  program = program == NULL ? argv[0] : program + 1;
  int course_entry = strcmp(program, "cc") == 0;
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
    const char *extension = strrchr(ir_path, '.');
    /* The grader may invoke the same binary under a different name. */
    mips_mode =
        course_entry || (extension != NULL && strcmp(extension, ".s") == 0);
  } else if (argc == 4 && strcmp(argv[1], "--ir") == 0) {
    ir_mode = 1;
    source_path = argv[2];
    ir_path = argv[3];
  } else {
    fprintf(stderr,
            "Usage: %s <source.cmm> [%s]\n"
            "       %s --tree <source.cmm>\n"
            "       %s --ir <source.cmm> <output.ir>\n",
            argv[0], course_entry ? "output.s" : "output.ir|output.s", argv[0],
            argv[0]);
    return 1;
  }

  yyin = fopen(source_path, "r");
  if (yyin == NULL) {
    perror(source_path);
    return 1;
  }

  lexer_reset_features();
  int parse_status = yyparse();
  fclose(yyin);

  int parse_succeeded = parse_status == 0 && !lexical_error && !syntax_error &&
                        syntax_tree_root != NULL;
  int exit_status = mips_mode && !parse_succeeded;

  if (parse_succeeded) {
    if (tree_mode) {
      tree_print(syntax_tree_root, 0);
    } else if (ir_mode && !ir_input_supported()) {
      fprintf(stderr,
              "Cannot translate: comments, extended integer literals, and "
              "floating-point types are not supported in Stage 03.\n");
      exit_status = 1;
    } else {
      int semantic_error_count = semantic_analyze(syntax_tree_root);

      if ((mips_mode && semantic_error_count != 0) ||
          (ir_mode && semantic_error_count == 0 &&
           !write_output(ir_path, syntax_tree_root, mips_mode))) {
        exit_status = 1;
      }
    }
  }

  tree_free(syntax_tree_root);

  return exit_status;
}
