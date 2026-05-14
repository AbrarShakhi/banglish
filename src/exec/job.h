#pragma once
#include <sys/types.h>

typedef enum {
	JOB_RUNNING,
	JOB_STOPPED,
	JOB_DONE,
} JobStatus;
