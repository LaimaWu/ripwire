class Lane:
    """A lane of the pool: it schedules its jobs in order."""

    def __init__(self):
        self.jobs = []

    def schedule_lane(self, job):
        """Schedule one job onto this lane."""
        self.jobs.append(job)
        return len(self.jobs)


def run_lane():
    lane = Lane()
    return lane.schedule_lane("job")
