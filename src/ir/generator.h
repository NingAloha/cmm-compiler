#ifndef GENERATOR_H
#define GENERATOR_H

#include "ir.h"
#include "../frontend/tree.h"

#include <stdio.h>

typedef struct IrVariable IrVariable;

typedef struct {
  IrContext ir;
  IrVariable *variables;
  unsigned int next_variable;
  int failed;
} IrGenerator;

void ir_generator_init(IrGenerator *generator, FILE *output);
void ir_generator_clear(IrGenerator *generator);

int ir_generate_program(IrGenerator *generator, const TreeNode *root);

#endif
