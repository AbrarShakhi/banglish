#pragma once
#include <sys/types.h>

typedef enum {
	JOB_RUNNING,
	JOB_STOPPED,
	JOB_DONE,
} JobStatus;

typedef struct Job {
	int jid;    /* 1-based job ID              */
	pid_t pgid; /* process group ID            */
	JobStatus status;
	int notified;  /* 1 if user was told it ended */
	char *cmdline; /* display string; heap-owned  */
	struct Job *next;
} Job;

typedef struct JobTable {
	Job *head;
	int next_jid;
} JobTable;

JobTable *jobtable_create(void);
void jobtable_destroy(JobTable *jt);
Job *job_add(JobTable *jt, pid_t pgid, const char *cmdline);
Job *job_find_jid(JobTable *jt, int jid);
Job *job_find_pgid(JobTable *jt, pid_t pgid);
void job_remove(JobTable *jt, int jid);
void job_update_status(JobTable *jt); /* reap finished jobs, WNOHANG */
void job_print_all(JobTable *jt, int fd);
void job_set_foreground(JobTable *jt, Job *j, int cont); /* tcsetpgrp + wait */
