#include "ir.h"

#include <stdarg.h>
#include <stdio.h>

void ir_context_init(IrContext *context, FILE *output) {
  if (context == NULL) {
    return;
  }

  context->output = output;
  context->next_temp = 1;
  context->next_label = 1;
  context->failed = output == NULL;
}

int ir_emit(IrContext *context, const char *format, ...) {
  if (context == NULL || context->output == NULL || context->failed ||
      format == NULL) {
    return 0;
  }

  va_list arguments;
  va_start(arguments, format);

  int written = vfprintf(context->output, format, arguments);

  va_end(arguments);

  if (written < 0 || fputc('\n', context->output) == EOF) {
    context->failed = 1;
    return 0;
  }

  return 1;
}

int ir_new_temp(IrContext *context, char *buffer, size_t buffer_size) {
  if (context == NULL || buffer == NULL || buffer_size == 0 ||
      context->failed) {
    return 0;
  }

  int written = snprintf(buffer, buffer_size, "t%u", context->next_temp);

  if (written < 0 || (size_t)written >= buffer_size) {
    return 0;
  }

  context->next_temp++;
  return 1;
}

int ir_new_label(IrContext *context, char *buffer, size_t buffer_size) {
  if (context == NULL || buffer == NULL || buffer_size == 0 ||
      context->failed) {
    return 0;
  }

  int written = snprintf(buffer, buffer_size, "label%u", context->next_label);

  if (written < 0 || (size_t)written >= buffer_size) {
    return 0;
  }

  context->next_label++;
  return 1;
}