#ifndef LEXER_H
#define LEXER_H

extern int lexical_error;
extern int source_has_comment;
extern int source_has_extended_integer;
extern int source_has_float;

void lexer_reset_features(void);

#endif
