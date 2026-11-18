#ifndef GENERATOR_H
#define GENERATOR_H

#include "../frontend/tree.h"
#include "ir.h"

#include <stdio.h>

typedef struct IrVariable IrVariable;

typedef struct {
  IrContext ir;
  IrVariable *variables;
  unsigned int next_variable;
  int failed;
} IrGenerator;

/** Initializes a generator that borrows the output stream. */
void ir_generator_init(IrGenerator *generator, FILE *output);

/** Releases generator-owned state without closing the output stream. */
void ir_generator_clear(IrGenerator *generator);

/**
 * Emits IR for a semantically checked syntax tree without taking ownership.
 * Returns 1 on success or 0 on failure; failure may leave partial output.
 */
int ir_generate_program(IrGenerator *generator, const TreeNode *root);

#endif
