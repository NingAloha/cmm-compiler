#ifndef LEXER_H
#define LEXER_H

/* These flags stay set once the corresponding condition is seen. */
extern int lexical_error;
extern int source_has_comment;
extern int source_has_extended_integer;
extern int source_has_float;

/* Clears only the flags above, not the scanner buffer, state, or line number.
 */
void lexer_reset_features(void);

#endif
