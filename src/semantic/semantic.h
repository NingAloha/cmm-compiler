#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "../frontend/tree.h"

/* Analyze a parsed program and return the number of semantic errors. */

int semantic_analyze(const TreeNode *root);

#endif
