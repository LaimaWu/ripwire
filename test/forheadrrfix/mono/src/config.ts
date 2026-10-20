// Scheduler settings.
export function schedulerWorkerCount(env: Record<string, string>): number {
  return Number(env["WORKERS"] ?? "4");
}
