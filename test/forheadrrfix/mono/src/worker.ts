// A worker runs one job at a time.
export function runWorkerJob(worker: string, job: string): string {
  return worker + " ran " + job;
}
