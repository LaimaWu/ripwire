// The scheduler: queued jobs are dispatched to workers.
export function dispatchQueuedJobs(queue: string[], workers: string[]): number {
  let dispatched = 0;
  for (const job of queue) { dispatchJobToWorker(job, workers); dispatched++; }
  return dispatched;
}
export function scheduleJobDispatch(job: string, delayMs: number): void {
  setTimeout(() => dispatchQueuedJobs([job], ["worker"]), delayMs);
}
export function dispatchJobToWorker(job: string, workers: string[]): string {
  const worker = workers[job.length % workers.length];
  return worker + ":" + job;
}
export function requeueJobsForDispatch(queue: string[], failed: string[]): string[] {
  return queue.concat(failed);
}
export function schedulerDispatchLoop(queue: string[], workers: string[]): void {
  while (queue.length > 0) { dispatchQueuedJobs(queue.splice(0, 4), workers); }
}
export function dispatchWorkerJobs(worker: string, jobs: string[]): number {
  return jobs.filter((job) => job.startsWith(worker)).length;
}
export function queueJobsForScheduler(jobs: string[]): string[] {
  return jobs.slice().sort();
}
export function cancelQueuedJobDispatch(queue: string[], job: string): string[] {
  return queue.filter((queued) => queued !== job);
}
export function dispatchQueuedJobsToIdleWorkers(queue: string[], idle: string[]): number {
  return dispatchQueuedJobs(queue, idle);
}
export function schedulerDispatchQueuedJob(job: string, workers: string[]): string {
  return dispatchJobToWorker(job, workers);
}
export function retryQueuedJobDispatch(job: string, workers: string[]): string {
  return dispatchJobToWorker(job, workers.slice().reverse());
}
export function dispatchQueuedJobsInBatches(queue: string[], workers: string[], size: number): void {
  for (let i = 0; i < queue.length; i += size) { dispatchQueuedJobs(queue.slice(i, i + size), workers); }
}
