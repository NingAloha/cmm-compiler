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

static int ir_new_name(char *buffer, size_t buffer_size, const char *prefix,
                       unsigned int *next) {
  if (buffer == NULL || buffer_size == 0) {
    return 0;
  }

  int written = snprintf(buffer, buffer_size, "%s%u", prefix, *next);
  if (written < 0 || (size_t)written >= buffer_size) {
    return 0;
  }

  ++*next;
  return 1;
}

int ir_new_temp(IrContext *context, char *buffer, size_t buffer_size) {
  return context != NULL && !context->failed &&
         ir_new_name(buffer, buffer_size, "t", &context->next_temp);
}

int ir_new_label(IrContext *context, char *buffer, size_t buffer_size) {
  return context != NULL && !context->failed &&
         ir_new_name(buffer, buffer_size, "label", &context->next_label);
}
