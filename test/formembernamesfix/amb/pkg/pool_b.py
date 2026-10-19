"""Worker pool B: another worker of the same name, for remote tasks."""


class TaskWorker:
    """Runs remote tasks."""

    def send_task(self, task):
        return ("remote", task)

    def close(self):
        return None
