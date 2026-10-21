"""Worker pool A: a worker that runs queued tasks."""


class TaskWorker:
    """Runs queued tasks one at a time."""

    def run_task(self, task):
        return task()

    def stop(self):
        return None


class TaskQueue:
    """The one queue of tasks the workers share."""

    def put_task(self, task):
        return task

    def take(self):
        return None
