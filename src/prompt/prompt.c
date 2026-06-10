#include "prompt.h"
#include "core/env.h"
#include "info/info.h"
#include "utils/memory.h"
#include "utils/str.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_READLINE
#include "completion.h"
#include <readline/history.h>
#include <readline/readline.h>
#endif

/* ---- ANSI helpers ---- */
#define ANSI_RESET "\033[0m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_BOLD "\033[1m"

/* Readline needs non-printing chars wrapped in \001...\002 */
#ifdef HAVE_READLINE
#define RL_RESET "\001" ANSI_RESET "\002"
#define RL_RED "\001" ANSI_RED "\002"
#define RL_GREEN "\001" ANSI_GREEN "\002"
#define RL_YELLOW "\001" ANSI_YELLOW "\002"
#define RL_BLUE "\001" ANSI_BLUE "\002"
#define RL_MAGENTA "\001" ANSI_MAGENTA "\002"
#define RL_BOLD "\001" ANSI_BOLD "\002"
#else
#define RL_RESET ANSI_RESET
#define RL_RED ANSI_RED
#define RL_GREEN ANSI_GREEN
#define RL_YELLOW ANSI_YELLOW
#define RL_BLUE ANSI_BLUE
#define RL_MAGENTA ANSI_MAGENTA
#define RL_BOLD ANSI_BOLD
#endif

/* Build prompt string: [status] user@host: ~/cwd %  */
static char *build_prompt(ShellState *sh) {
  char *user = info_get_username();
  char *host = info_get_hostname();
  char *cwd = info_get_cwd_tilde();

  char status_buf[64];
  const char *sc = sh->last_status == 0 ? RL_GREEN : RL_RED;
  snprintf(status_buf, sizeof(status_buf), "%s[%d]" RL_RESET " ", sc,
           sh->last_status);

  char *uh = str_concat3(RL_GREEN RL_BOLD, user, RL_RESET "@" RL_BLUE);
  char *uh2 = str_concat(uh, host);
  char *cw = str_concat3(RL_RESET ": " RL_YELLOW, cwd, RL_RESET);
  char *p1 = str_concat(status_buf, uh2);
  char *p2 = str_concat(p1, cw);
  char *ps = str_concat(p2, RL_MAGENTA " %%" RL_RESET " ");

  mem_free(user);
  mem_free(host);
  mem_free(cwd);
  mem_free(uh);
  mem_free(uh2);
  mem_free(cw);
  mem_free(p1);
  mem_free(p2);
  return ps;
}

void prompt_init(ShellState *sh) {
#ifdef HAVE_READLINE
  completion_init(sh);
  if (sh->history_path)
    read_history(sh->history_path);
  rl_bind_key('\t', rl_complete);
  using_history();
#else
  (void)sh;
#endif
}

void prompt_teardown(ShellState *sh) {
#ifdef HAVE_READLINE
  if (sh && sh->history_path)
    write_history(sh->history_path);
#else
  (void)sh;
#endif
}

char *prompt_readline(ShellState *sh) {
  char *ps = sh->secondary_prompt ? mem_strdup(RL_BLUE "> " RL_RESET)
                                  : build_prompt(sh);

#ifdef HAVE_READLINE
  char *line = readline(ps);
  mem_free(ps);
  if (line && *line)
    add_history(line);
  return line; /* malloc-allocated; caller frees with free() */
#else
  /* Fallback: print prompt and use fgets */
  fputs(ps, stdout);
  fflush(stdout);
  mem_free(ps);

  char buf[4096];
  if (!fgets(buf, sizeof(buf), stdin))
    return NULL;
  size_t len = strlen(buf);
  if (len > 0 && buf[len - 1] == '\n')
    buf[len - 1] = '\0';
  /* Return malloc-allocated string for consistency with readline API */
  return strdup(buf);
#endif
}
