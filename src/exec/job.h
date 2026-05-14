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
