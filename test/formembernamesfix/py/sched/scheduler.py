"""Job scheduling: the scheduler hands jobs to workers."""


class JobScheduler:
    """Queues jobs and hands them to idle workers."""

    def __init__(self, workers):
        self.workers = list(workers)
        self.queue = []

    def submit(self, job):
        def wrap_job(inner):
            return ("job", inner)

        self.queue.append(wrap_job(job))

    def assign(self):
        while self.queue and self.workers:
            worker = self.workers.pop()
            worker.run(self.queue.pop(0))

    def drain(self):
        pending = self.queue
        self.queue = []
        return pending


def schedule_jobs(scheduler, jobs):
    for job in jobs:
        scheduler.submit(job)
    scheduler.assign()
