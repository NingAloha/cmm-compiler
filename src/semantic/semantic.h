#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "../frontend/tree.h"

/* Analyze a borrowed Program tree without modifying it. Each call starts
 * with a fresh symbol table, prints semantic diagnostics to stdout, and
 * returns the number emitted. A NULL root emits nothing and returns 0. */

int semantic_analyze(const TreeNode *root);

#endif
