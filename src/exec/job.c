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
