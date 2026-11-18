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

/** Initializes counters and borrows output without taking ownership. */
void ir_context_init(IrContext *context, FILE *output);

/**
 * Writes one formatted instruction and appends a newline.
 *
 * Returns 1 on success or 0 on failure. A stream write failure prevents
 * subsequent emission; the caller must still check for errors on close.
 */
int ir_emit(IrContext *context, const char *format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

/** Returns 1 and advances the temporary counter only if the name fits. */
int ir_new_temp(IrContext *context, char *buffer, size_t buffer_size);

/** Returns 1 and advances the label counter only if the name fits. */
int ir_new_label(IrContext *context, char *buffer, size_t buffer_size);

#endif
