#include "signal.h"
#include "exec/job.h"
#include "shell.h"
#include <signal.h>
#include <unistd.h>

static void sigchld_handler(int sig) {
  (void)sig;
  ShellState *sh = shell_get_global();
  if (sh && sh->jobs)
    job_update_status(sh->jobs);
}

void signals_init_interactive(void) {
  /*
   * Do NOT ignore SIGINT — readline installs its own SIGINT handler that
   * clears the line and re-prompts, which is the correct interactive behavior.
   * We only ignore signals that would stop or orphan the shell's process group.
   */
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);

  /* Reap background children asynchronously */
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGCHLD, &sa, NULL);

  /* Put the shell in its own process group if it isn't already */
  pid_t shell_pgid = getpid();
  if (getpgrp() != shell_pgid)
    setpgid(0, 0);
  tcsetpgrp(STDIN_FILENO, shell_pgid);
}

void signals_init_child(void) {
  signal(SIGINT, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGTSTP, SIG_DFL);
  signal(SIGTTIN, SIG_DFL);
  signal(SIGTTOU, SIG_DFL);
  signal(SIGCHLD, SIG_DFL);
}

void signals_init_script(void) {
  signal(SIGINT, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGCHLD, SIG_DFL);
}
