#ifndef SEMANTIC_INTERNAL_H
#define SEMANTIC_INTERNAL_H

/* Private interface shared by the semantic analysis translation units. */

#include "semantic.h"
#include "symbol.h"
#include "type.h"

typedef struct SemanticContext {
	SymbolTable symbols;
	Type *current_return_type;
	int error_count;
} SemanticContext;

/* semantic.c: context and error reporting */

void report_error(SemanticContext *context, int error_type, int line,
                  const char *message);

/* sem_decl.c: types, declarations and functions */

Type *analyze_specifier(SemanticContext *context, const TreeNode *node);
void analyze_def_list(SemanticContext *context, const TreeNode *node);
void analyze_ext_dec_list(SemanticContext *context, const TreeNode *node,
                          Type *base_type);
void analyze_fun_dec(SemanticContext *context, const TreeNode *node,
                     Type *return_type, const TreeNode *body);

/* sem_stmt.c: statements */

void analyze_comp_st(SemanticContext *context, const TreeNode *node);

/* sem_expr.c: expressions */

Type *analyze_exp(SemanticContext *context, const TreeNode *node);

#endif
