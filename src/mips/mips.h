#ifndef MIPS_H
#define MIPS_H

#include <stdio.h>

/* Read Stage03 text IR from its current position (rewind it before calling).
 * Return 1 on success, 0 on invalid IR or a resource/I/O error. The caller
 * retains ownership of both streams. */
int mips_generate(FILE *ir, FILE *output);

#endif
