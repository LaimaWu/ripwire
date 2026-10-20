/* The job scheduler: a run queue of jobs. */
typedef struct job_sched job_sched;
typedef struct job_entry { int id; int priority; } job_entry;
