#include "job.h"

#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils/memory.h"

JobTable *jobtable_create(void) {
  JobTable *jt = mem_alloc(sizeof(JobTable));
  jt->head = NULL;
  jt->next_jid = 1;
  return jt;
}

void jobtable_destroy(JobTable *jt) {
  Job *j = jt->head;
  while (j) {
    Job *next = j->next;
    mem_free(j->cmdline);
    mem_free(j);
    j = next;
  }
  mem_free(jt);
}

Job *job_add(JobTable *jt, pid_t pgid, const char *cmdline) {
  Job *j = mem_alloc(sizeof(Job));
  j->jid = jt->next_jid++;
  j->pgid = pgid;
  j->status = JOB_RUNNING;
  j->notified = 0;
  j->cmdline = mem_strdup(cmdline);
  j->next = jt->head;
  jt->head = j;
  return j;
}

Job *job_find_jid(JobTable *jt, int jid) {
  for (Job *j = jt->head; j; j = j->next)
    if (j->jid == jid)
      return j;
  return NULL;
}

Job *job_find_pgid(JobTable *jt, pid_t pgid) {
  for (Job *j = jt->head; j; j = j->next)
    if (j->pgid == pgid)
      return j;
  return NULL;
}

void job_remove(JobTable *jt, int jid) {
  Job **pp = &jt->head;
  while (*pp) {
    if ((*pp)->jid == jid) {
      Job *dead = *pp;
      *pp = dead->next;
      mem_free(dead->cmdline);
      mem_free(dead);
      return;
    }
    pp = &(*pp)->next;
  }
}

/* Reap finished background jobs (non-blocking). */
void job_update_status(JobTable *jt) {
  int wstatus;
  pid_t pid;
  while ((pid = waitpid(-1, &wstatus, WNOHANG | WUNTRACED)) > 0) {
    /* Find the job by pgid: a job's pgid == first child's pid for simple
     * cmds */
    Job *j = job_find_pgid(jt, pid);
    if (!j) {
      /* Might be one process in a multi-process pipeline: search all jobs
       */
      for (j = jt->head; j; j = j->next)
        if (j->pgid == (pid_t)pid)
          break;
    }
    if (!j)
      continue;

    if (WIFSTOPPED(wstatus))
      j->status = JOB_STOPPED;
    else
      j->status = JOB_DONE;
  }
}

void job_print_all(JobTable *jt, int fd) {
  FILE *out = fd == 1 ? stdout : stderr;
  /* Print in reverse order (lowest jid first) by collecting into array */
  int count = 0;
  for (Job *j = jt->head; j; j = j->next)
    count++;
  if (!count)
    return;
  Job **arr = mem_alloc((size_t)count * sizeof(Job *));
  int idx = 0;
  for (Job *j = jt->head; j; j = j->next)
    arr[idx++] = j;
  /* Reverse */
  for (int i = 0, k = count - 1; i < k; i++, k--) {
    Job *tmp = arr[i];
    arr[i] = arr[k];
    arr[k] = tmp;
  }
  for (int i = 0; i < count; i++) {
    Job *j = arr[i];
    const char *st = j->status == JOB_RUNNING   ? "Running"
                     : j->status == JOB_STOPPED ? "Stopped"
                                                : "Done";
    fprintf(out, "[%d] %s\t%s\n", j->jid, st, j->cmdline);
  }
  mem_free(arr);
}

/* Bring job j to the foreground. cont: send SIGCONT if stopped. */
void job_set_foreground(JobTable *jt, Job *j, int cont) {
  /* Give the terminal to the job's process group */
  tcsetpgrp(STDIN_FILENO, j->pgid);
  if (cont) {
    j->status = JOB_RUNNING;
    kill(-j->pgid, SIGCONT);
  }
  /* Wait for all processes in the group */
  int wstatus;
  pid_t pid;
  while ((pid = waitpid(-j->pgid, &wstatus, WUNTRACED)) > 0) {
    if (WIFSTOPPED(wstatus)) {
      j->status = JOB_STOPPED;
      break;
    }
    if (WIFEXITED(wstatus) || WIFSIGNALED(wstatus)) {
      j->status = JOB_DONE;
    }
  }
  /* Give terminal back to the shell */
  tcsetpgrp(STDIN_FILENO, getpgrp());
  (void)jt;
}
