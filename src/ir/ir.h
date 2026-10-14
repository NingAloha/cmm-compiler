#ifndef IR_H
#define IR_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
  FILE *output;
  unsigned int next_temp;
  unsigned int next_label;
  int failed;
} IrContext;

void ir_context_init(IrContext *context, FILE *output);

int ir_emit(IrContext *context, const char *format, ...);

int ir_new_temp(IrContext *context, char *buffer, size_t buffer_size);

int ir_new_label(IrContext *context, char *buffer, size_t buffer_size);

#endif