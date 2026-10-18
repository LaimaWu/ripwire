#include "sched.h"
/* The job scheduler's state: the run queue of jobs and its length. */
struct job_sched { job_entry jobs[64]; int count; int running; };
/* Initialise the job scheduler: an empty run queue of jobs. */
void job_sched_init(struct job_sched *s) { s->count = 0; s->running = 0; }
/* Run the job scheduler: run each queued job in order. */
void job_sched_run(struct job_sched *s) { for (int i = 0; i < s->count; i++) { s->running = s->jobs[i].id; } }
/* Queue a job on the job scheduler's run queue. */
int job_sched_queue(struct job_sched *s, job_entry job) { if (s->count >= 64) return -1; s->jobs[s->count++] = job; return 0; }
/* Count the jobs the job scheduler still has queued to run. */
int job_sched_pending(const struct job_sched *s) { return s->count; }
