// The queue of jobs waiting for the scheduler.
export function drainQueuedJobs(queue: string[]): string[] {
  return queue.splice(0, queue.length);
}
