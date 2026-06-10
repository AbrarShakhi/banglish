#include "expand.h"
#include "core/env.h"
#include "utils/memory.h"
#include "utils/str.h"
#include "utils/vec.h"
#include <ctype.h>
#include <glob.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Forward declaration to break circular dep: expand.c → eval.c */
int eval_string(ShellState *sh, const char *src);

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static char *expand_tilde(const char *word) {
  if (word[0] != '~')
    return mem_strdup(word);

  const char *rest = word + 1;
  if (*rest == '\0' || *rest == '/') {
    const char *home = getenv("HOME");
    if (!home) {
      struct passwd *pw = getpwuid(getuid());
      home = pw ? pw->pw_dir : "";
    }
    if (*rest == '\0')
      return mem_strdup(home);
    return str_concat(home, rest);
  }
  /* ~username */
  const char *slash = strchr(rest, '/');
  size_t ulen = slash ? (size_t)(slash - rest) : strlen(rest);
  char *uname = mem_strndup(rest, ulen);
  struct passwd *pw = getpwnam(uname);
  mem_free(uname);
  if (!pw)
    return mem_strdup(word);
  if (!slash)
    return mem_strdup(pw->pw_dir);
  return str_concat(pw->pw_dir, slash);
}

/* Expand $VAR, ${VAR}, $?, $#, $0..$N inside a string (no glob). */
static char *expand_params(const char *s, ShellState *sh) {
  Vec *out = vec_create();

#define OUT_PUSH(c)                                                            \
  do {                                                                         \
    char *_c = mem_alloc(1);                                                   \
    *_c = (c);                                                                 \
    vec_push(out, _c);                                                         \
  } while (0)

  size_t i = 0, len = strlen(s);
  while (i < len) {
    char c = s[i];
    if (c != '$') {
      OUT_PUSH(c);
      i++;
      continue;
    }
    i++; /* skip $ */
    if (i >= len) {
      OUT_PUSH('$');
      continue;
    }

    char nc = s[i];

    if (nc == '{') {
      /* ${NAME} */
      i++;
      size_t start = i;
      while (i < len && s[i] != '}')
        i++;
      char *name = mem_strndup(s + start, i - start);
      const char *val = env_get(sh->env, name);
      if (!val)
        val = "";
      for (size_t k = 0; val[k]; k++)
        OUT_PUSH(val[k]);
      mem_free(name);
      if (i < len)
        i++; /* skip } */

    } else if (nc == '(') {
      /* $(cmd): run command, capture stdout */
      i++;
      /* Find matching ) accounting for nested () and quotes */
      int depth = 1;
      size_t start = i;
      while (i < len && depth > 0) {
        if (s[i] == '(')
          depth++;
        else if (s[i] == ')')
          depth--;
        else if (s[i] == '\'' || s[i] == '"') {
          char q = s[i++];
          while (i < len && s[i] != q) {
            if (s[i] == '\\')
              i++;
            i++;
          }
        }
        if (depth > 0)
          i++;
      }
      size_t cmd_len = i - start;
      if (i < len)
        i++; /* skip closing ) */

      char *cmd_str = mem_strndup(s + start, cmd_len);

      /* Capture output via pipe */
      int pfd[2];
      if (pipe(pfd) == 0) {
        fflush(NULL); /* flush stdio before fork to avoid dup in child */
        pid_t pid = fork();
        if (pid == 0) {
          close(pfd[0]);
          dup2(pfd[1], STDOUT_FILENO);
          close(pfd[1]);
          /* Run the command string in a subshell */
          int status = eval_string(sh, cmd_str);
          fflush(stdout);
          _exit(status);
        }
        close(pfd[1]);
        /* Read output */
        char rbuf[4096];
        ssize_t nr;
        Vec *cap = vec_create();
        while ((nr = read(pfd[0], rbuf, sizeof(rbuf))) > 0) {
          for (ssize_t k = 0; k < nr; k++) {
            char *bc = mem_alloc(1);
            *bc = rbuf[k];
            vec_push(cap, bc);
          }
        }
        close(pfd[0]);
        int wstatus;
        waitpid(pid, &wstatus, 0);

        /* Trim trailing newlines */
        while (cap->count > 0 && *(char *)cap->data[cap->count - 1] == '\n') {
          mem_free(cap->data[--cap->count]);
        }

        for (int k = 0; k < cap->count; k++) {
          OUT_PUSH(*(char *)cap->data[k]);
          mem_free(cap->data[k]);
        }
        vec_destroy(cap);
      }
      mem_free(cmd_str);

    } else if (nc == '?') {
      /* $? — last exit status */
      char buf[16];
      snprintf(buf, sizeof(buf), "%d", sh->last_status);
      for (size_t k = 0; buf[k]; k++)
        OUT_PUSH(buf[k]);
      i++;

    } else if (nc == '#') {
      /* $# — number of positional params */
      char buf[16];
      snprintf(buf, sizeof(buf), "%d",
               sh->script_argc > 0 ? sh->script_argc - 1 : 0);
      for (size_t k = 0; buf[k]; k++)
        OUT_PUSH(buf[k]);
      i++;

    } else if (nc >= '0' && nc <= '9') {
      /* $0..$9 */
      int idx = nc - '0';
      i++;
      const char *val = "";
      if (idx < sh->script_argc && sh->script_argv)
        val = sh->script_argv[idx];
      for (size_t k = 0; val[k]; k++)
        OUT_PUSH(val[k]);

    } else if (isalpha((unsigned char)nc) || nc == '_') {
      /* $NAME */
      size_t start = i;
      while (i < len && (isalnum((unsigned char)s[i]) || s[i] == '_'))
        i++;
      char *name = mem_strndup(s + start, i - start);
      const char *val = env_get(sh->env, name);
      if (!val)
        val = "";
      for (size_t k = 0; val[k]; k++)
        OUT_PUSH(val[k]);
      mem_free(name);

    } else {
      /* Bare $ — keep as-is */
      OUT_PUSH('$');
    }
  }

  OUT_PUSH('\0');
  size_t slen = (size_t)out->count;
  char *result = mem_alloc(slen);
  for (int j = 0; j < out->count; j++) {
    result[j] = *(char *)out->data[j];
    mem_free(out->data[j]);
  }
  vec_destroy(out);

#undef OUT_PUSH
  return result;
}

/* Strip quote delimiters (single and double quotes) from a string.
   Variables have already been expanded, so only remove the quote chars. */
static char *remove_quotes(const char *s) {
  Vec *out = vec_create();
#define OUT_PUSH(c)                                                            \
  do {                                                                         \
    char *_c = mem_alloc(1);                                                   \
    *_c = (c);                                                                 \
    vec_push(out, _c);                                                         \
  } while (0)
  size_t i = 0, len = strlen(s);
  while (i < len) {
    char c = s[i];
    if (c == '\'') {
      i++;
      while (i < len && s[i] != '\'')
        OUT_PUSH(s[i++]);
      if (i < len)
        i++; /* closing ' */
    } else if (c == '"') {
      i++;
      while (i < len && s[i] != '"') {
        if (s[i] == '\\' && i + 1 < len)
          i++;
        OUT_PUSH(s[i++]);
      }
      if (i < len)
        i++; /* closing " */
    } else if (c == '\\' && i + 1 < len) {
      i++;
      OUT_PUSH(s[i++]);
    } else {
      OUT_PUSH(s[i++]);
    }
  }
  OUT_PUSH('\0');
#undef OUT_PUSH
  size_t slen = (size_t)out->count;
  char *result = mem_alloc(slen);
  for (int j = 0; j < out->count; j++) {
    result[j] = *(char *)out->data[j];
    mem_free(out->data[j]);
  }
  vec_destroy(out);
  return result;
}

/* Perform glob on a pattern. Returns NULL-terminated array.
   If no match, returns array containing the original pattern. */
static char **do_glob(const char *pattern) {
  glob_t g;
  int ret = glob(pattern, GLOB_NOCHECK, NULL, &g);
  char **result;
  if (ret == 0 && g.gl_pathc > 0) {
    result = mem_alloc((g.gl_pathc + 1) * sizeof(char *));
    for (size_t k = 0; k < g.gl_pathc; k++)
      result[k] = mem_strdup(g.gl_pathv[k]);
    result[g.gl_pathc] = NULL;
  } else {
    result = mem_alloc(2 * sizeof(char *));
    result[0] = mem_strdup(pattern);
    result[1] = NULL;
  }
  globfree(&g);
  return result;
}

/* Check if a word contains glob characters outside of quotes */
static int has_glob_chars(const char *s) {
  int in_sq = 0, in_dq = 0;
  for (size_t i = 0; s[i]; i++) {
    if (s[i] == '\'' && !in_dq) {
      in_sq = !in_sq;
      continue;
    }
    if (s[i] == '"' && !in_sq) {
      in_dq = !in_dq;
      continue;
    }
    if (s[i] == '\\' && !in_sq && s[i + 1]) {
      i++;
      continue;
    }
    if (!in_sq && !in_dq) {
      if (s[i] == '*' || s[i] == '?' || s[i] == '[')
        return 1;
    }
  }
  return 0;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

char **word_expand(const char *word, ShellState *sh) {
  /* Step 1: tilde */
  char *after_tilde = expand_tilde(word);

  /* Step 2+3: parameter + command substitution */
  char *after_params = expand_params(after_tilde, sh);
  mem_free(after_tilde);

  /* Step 4: glob (only if the string has glob chars) */
  char **globbed;
  if (has_glob_chars(after_params)) {
    /* Remove quotes before globbing */
    char *no_q = remove_quotes(after_params);
    globbed = do_glob(no_q);
    mem_free(no_q);
    mem_free(after_params);
    /* Words from glob already have no quotes */
    return globbed;
  }

  /* Step 5: quote removal */
  char *final = remove_quotes(after_params);
  mem_free(after_params);

  char **result = mem_alloc(2 * sizeof(char *));
  result[0] = final;
  result[1] = NULL;
  return result;
}

void expand_free_words(char **words) {
  if (!words)
    return;
  for (int i = 0; words[i]; i++)
    mem_free(words[i]);
  mem_free(words);
}

char **argv_expand(char **argv, int argc, ShellState *sh) {
  Vec *out = vec_create();
  for (int i = 0; i < argc; i++) {
    char **expanded = word_expand(argv[i], sh);
    for (int k = 0; expanded[k]; k++)
      vec_push(out, expanded[k]);
    mem_free(expanded); /* free the array, items are owned by 'out' */
  }
  char **result = (char **)vec_to_null_terminated(out);
  vec_destroy(out);
  return result;
}

char *expand_variables_only(const char *s, ShellState *sh) {
  char *expanded = expand_params(s, sh);
  char *clean = remove_quotes(expanded);
  mem_free(expanded);
  return clean;
}
