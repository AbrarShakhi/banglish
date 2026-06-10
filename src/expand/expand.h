#pragma once
#include "core/shell.h"

/*
 * Expand a single raw word through the full expansion pipeline:
 *   tilde → parameter → command-substitution → glob → quote-removal
 *
 * Returns a NULL-terminated array of expanded strings (one word may glob
 * into multiple). Caller must free with expand_free_words().
 */
char **word_expand(const char *word, ShellState *sh);
void expand_free_words(char **words);

/* Expand all argv words and collect into a flat NULL-terminated array.
   Returns newly allocated array; caller frees with expand_free_words(). */
char **argv_expand(char **argv, int argc, ShellState *sh);

/* Expand variable references in a string without glob. Returns heap string. */
char *expand_variables_only(const char *s, ShellState *sh);
