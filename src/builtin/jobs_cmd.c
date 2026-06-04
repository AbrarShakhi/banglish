#include "jobs_cmd.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "exec/job.h"

int builtin_jobs(ShellState *sh, int argc, char **argv) {
  (void)argc;
  (void)argv;
  job_update_status(sh->jobs);
  job_print_all(sh->jobs, 1);
  return 0;
}

int builtin_fg(ShellState *sh, int argc, char **argv) {
  int jid = -1;
  if (argc >= 2) {
    const char *spec = argv[1];
    if (spec[0] == '%')
      spec++;
    jid = atoi(spec);
  } else {
    /* Use most recent job */
    Job *j = sh->jobs->head;
    if (j)
      jid = j->jid;
  }
  if (jid < 0) {
    fprintf(stderr, "fg: no current job\n");
    return 1;
  }

  Job *j = job_find_jid(sh->jobs, jid);
  if (!j) {
    fprintf(stderr, "fg: %%%d: no such job\n", jid);
    return 1;
  }
  fprintf(stderr, "%s\n", j->cmdline);
  job_set_foreground(sh->jobs, j, 1);
  if (j->status == JOB_DONE)
    job_remove(sh->jobs, jid);
  return sh->last_status;
}

int builtin_bg(ShellState *sh, int argc, char **argv) {
  int jid = -1;
  if (argc >= 2) {
    const char *spec = argv[1];
    if (spec[0] == '%')
      spec++;
    jid = atoi(spec);
  } else {
    Job *j = sh->jobs->head;
    if (j)
      jid = j->jid;
  }
  if (jid < 0) {
    fprintf(stderr, "bg: no current job\n");
    return 1;
  }

  Job *j = job_find_jid(sh->jobs, jid);
  if (!j) {
    fprintf(stderr, "bg: %%%d: no such job\n", jid);
    return 1;
  }
  j->status = JOB_RUNNING;
  kill(-j->pgid, SIGCONT);
  fprintf(stderr, "[%d]+ %s &\n", j->jid, j->cmdline);
  return 0;
}
